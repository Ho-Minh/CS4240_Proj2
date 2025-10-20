#include "instruction_selector.hpp"
#include "frame_builder.hpp"
#include "emit_helpers.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ircpp {

    IRToMIPSSelector::IRToMIPSSelector() : regManager(), registry() {}


// ==========================
// SelectionContext helpers
// ==========================

std::string SelectionContext::generateLabel(const std::string& irLabel) {
    // 1) If we already mapped this IR label, return the cached MIPS label
    auto it = labelMap.find(irLabel);
    if (it != labelMap.end()) return it->second;

    // 2) Otherwise create a unique label. Use function prefix if available.
    //    Example: fib.L3 or L3 as a fallback
    std::ostringstream oss;
    if (!currentFunction.empty()) {
        oss << currentFunction << "_L" << labelCounter++;
    } else {
        oss << "L" << labelCounter++;
    }
    std::string mipsLabel = oss.str();

    // 3) Store and return
    labelMap.emplace(irLabel, mipsLabel);
    return mipsLabel;
}

// Optional: if your header declares these inside SelectionContext,
// keep this exact signature. If your header uses a different place for them,
// move accordingly.

std::vector<MIPSInstruction>
SelectionContext::generateFunctionPrologue(const IRFunction& func) {
    using namespace Registers;

    // Decide callee-saved set (except fp/ra which we handle explicitly)
    auto callee = regManager.getCalleeSavedRegs();
    std::vector<std::shared_ptr<Register>> saveRegs;
    saveRegs.reserve(callee.size());
    for (auto& r : callee) {
        const auto n = r->toString();
        if (n != "$fp" && n != "$ra") saveRegs.push_back(r);
    }

    // Very simple local sizing: 4 bytes per declared local variable.
    // If you have real layout, plug it in here.
    int localBytes = 0;
    if (!func.variables.empty()) localBytes = static_cast<int>(func.variables.size()) * 4;

    // Frame size: ra (4) + fp (4) + callee-saved s-registers + locals
    const int calleeBytes = static_cast<int>(saveRegs.size()) * 4;
    const int frameBytes  = 4 /*ra*/ + 4 /*fp*/ + calleeBytes + localBytes;

    std::vector<MIPSInstruction> out;
    out.reserve(4 + (int)saveRegs.size());

    // Tag current function for label generation
    currentFunction = func.name;

    // Allocate frame: addi sp, sp, -frameBytes
    out.emplace_back(
        MIPSOp::ADDI,
        /*label*/ func.name, // put function label on the first instruction
        std::vector<std::shared_ptr<MIPSOperand>>{ sp(), sp(), std::make_shared<Immediate>(-frameBytes) }
    );

    // Save ra and fp
    out.emplace_back(MIPSOp::SW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ ra(), std::make_shared<Address>(0, sp()) });
    out.emplace_back(MIPSOp::SW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ fp(), std::make_shared<Address>(4, sp()) });

    // Establish frame pointer: move fp, sp
    out.emplace_back(MIPSOp::MOVE, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ fp(), sp() });

    // Save callee-saved s-registers at 8(sp) upward (currently none if policy returns empty)
    int off = 8;
    for (auto& r : saveRegs) {
        out.emplace_back(MIPSOp::SW, "",
            std::vector<std::shared_ptr<MIPSOperand>>{ r, std::make_shared<Address>(off, sp()) });
        off += 4;
    }

    // Initialize function parameters: move $a0-$a3 into their allocated variable registers
    if (!func.parameters.empty()) {
        static std::shared_ptr<Register> aRegs[4] = { a0(), a1(), a2(), a3() };
        const size_t n = std::min<size_t>(4, func.parameters.size());
        for (size_t i = 0; i < n; ++i) {
            auto& p = func.parameters[i];
            if (!p) continue;
            auto dst = regManager.getRegister(p->getName());
            out.emplace_back(MIPSOp::MOVE, "",
                std::vector<std::shared_ptr<MIPSOperand>>{ dst, aRegs[i] });
        }
        // Note: params beyond 4 would be on caller's stack; not needed for current tests
    }

    // Do not auto-initialize locals; rely on IR assignments to set values

    // No explicit locals init; space is reserved by sp adjustment already.
    return out;
}

std::vector<MIPSInstruction>
SelectionContext::generateFunctionEpilogue(const IRFunction& func) {
    using namespace Registers;

    // Must mirror the prologue computations to restore from correct slots.
    auto callee = regManager.getCalleeSavedRegs();
    std::vector<std::shared_ptr<Register>> saveRegs;
    saveRegs.reserve(callee.size());
    for (auto& r : callee) {
        const auto n = r->toString();
        if (n != "$fp" && n != "$ra") saveRegs.push_back(r);
    }
    const int calleeBytes = static_cast<int>(saveRegs.size()) * 4;
    int localBytes = 0;
    if (!func.variables.empty()) localBytes = static_cast<int>(func.variables.size()) * 4;
    const int frameBytes = 4 /*ra*/ + 4 /*fp*/ + calleeBytes + localBytes;

    std::vector<MIPSInstruction> out;
    out.reserve(5 + (int)saveRegs.size());

    // Restore s-registers (same offsets used in prologue), BEFORE fixing sp.
    int off = 8;
    for (auto& r : saveRegs) {
        out.emplace_back(MIPSOp::LW, "",
            std::vector<std::shared_ptr<MIPSOperand>>{ r, std::make_shared<Address>(off, sp()) });
        off += 4;
    }

    // Restore fp and ra
    out.emplace_back(MIPSOp::LW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ fp(), std::make_shared<Address>(4, sp()) });
    out.emplace_back(MIPSOp::LW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ ra(), std::make_shared<Address>(0, sp()) });

    // Deallocate frame
    out.emplace_back(MIPSOp::ADDI, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ sp(), sp(), std::make_shared<Immediate>(frameBytes) });

    // Return
    out.emplace_back(MIPSOp::JR, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ ra() });

    return out;
}

// ==========================
// IRToMIPSSelector
// ==========================

// Simple stack-frame based emitter: map every variable to a frame slot and
// always load operands into temporaries before operations. Avoids register
// liveness across calls/branches.

namespace {
}

std::vector<MIPSInstruction>
IRToMIPSSelector::selectProgram(const IRProgram& program) {
    std::vector<MIPSInstruction> out;
    // Program entry: call main and then exit (syscall 10)
    out.emplace_back(MIPSOp::JAL, "", std::vector<std::shared_ptr<MIPSOperand>>{
        std::make_shared<Label>(std::string("main"))
    });
    out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::v0(), std::make_shared<Immediate>(10)
    });
    out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});

    for (const auto& fn : program.functions) {
        if (!fn) continue;
        // New simple emission path
        const IRFunction& F = *fn;
        // Build frame
        FrameInfo fi = buildFrame(F);

        // Prologue
        out.emplace_back(MIPSOp::ADDI, F.name, std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::sp(), Registers::sp(), std::make_shared<Immediate>(-fi.frameBytes)
        });
        out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::ra(), std::make_shared<Address>(0, Registers::sp())
        });
        out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::fp(), std::make_shared<Address>(4, Registers::sp())
        });
        out.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::fp(), Registers::sp()
        });

        // Store first 4 parameters to their slots (scalars copied; arrays store pointer)
        for (size_t i = 0; i < std::min<size_t>(4, F.parameters.size()); ++i) {
            auto p = F.parameters[i]; if (!p) continue;
            auto it = fi.varOffset.find(p->getName());
            if (it != fi.varOffset.end()) {
                static std::shared_ptr<Register> aRegs[4] = { Registers::a0(), Registers::a1(), Registers::a2(), Registers::a3() };
                out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                    aRegs[i], std::make_shared<Address>(it->second, Registers::fp())
                });
            }
        }

        // Load stack-passed parameters (beyond 4) into their slots
        if (F.parameters.size() > 4) {
            for (size_t i = 4; i < F.parameters.size(); ++i) {
                auto p = F.parameters[i]; if (!p) continue;
                int varOff = fi.varOffset[p->getName()];
                // Extra args are located immediately above the callee frame
                int extraOff = fi.frameBytes + int((i - 4) * 4);
                auto t = Registers::t0();
                out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                    t, std::make_shared<Address>(extraOff, Registers::fp())
                });
                out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                    t, std::make_shared<Address>(varOff, Registers::fp())
                });
            }
        }

        // Emit body
        SelectionContext ctx{ regManager };
        ctx.currentFunction = F.name;

        auto loadOp = [&](std::shared_ptr<IROperand> op, std::shared_ptr<Register> dst,
                          std::vector<MIPSInstruction>& code){
            if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(op)) {
                int val = std::stoi(c->getValueString());
                code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                    dst, std::make_shared<Immediate>(val)
                });
            } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
                // Scalar load (arrays are handled in array ops or passed as pointers in calls)
                if (std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                    // Load base pointer from slot (for array params) into dst
                    int off = fi.varOffset[v->getName()];
                    code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        dst, std::make_shared<Address>(off, Registers::fp())
                    });
                } else {
                    int off = fi.varOffset[v->getName()];
                    code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        dst, std::make_shared<Address>(off, Registers::fp())
                    });
                }
            }
        };

        auto storeVar = [&](const std::string& name, std::shared_ptr<Register> src,
                            std::vector<MIPSInstruction>& code){
            int off = fi.varOffset[name];
            code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                src, std::make_shared<Address>(off, Registers::fp())
            });
        };

        // Float helpers: store single from $fX and load single into $fX
        auto storeVarF32 = [&](const std::string& name, const std::shared_ptr<Register>& fSrc,
                               std::vector<MIPSInstruction>& code){
            int off = fi.varOffset[name];
            code.emplace_back(MIPSOp::S_S, "", std::vector<std::shared_ptr<MIPSOperand>>{
                fSrc, std::make_shared<Address>(off, Registers::fp())
            });
        };
        auto loadVarF32 = [&](const std::shared_ptr<IROperand>& op, const std::shared_ptr<Register>& fDst,
                              std::vector<MIPSInstruction>& code){
            if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
                int off = fi.varOffset[v->getName()];
                code.emplace_back(MIPSOp::L_S, "", std::vector<std::shared_ptr<MIPSOperand>>{
                    fDst, std::make_shared<Address>(off, Registers::fp())
                });
            } else {
                // Float immediates not supported here
            }
        };

        for (const auto& ir : F.instructions) {
            if (!ir) continue;
            std::vector<MIPSInstruction> code;
            switch (ir->opCode) {
                case IRInstruction::OpCode::LABEL: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    std::string L = qualLabel(F.name, lbl->getName());
                    code.emplace_back(MIPSOp::SLL, L, std::vector<std::shared_ptr<MIPSOperand>>{
                        Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0)
                    });
                    break;
                }
                case IRInstruction::OpCode::ASSIGN: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    if (!dst) break;
                    // Handle bulk array set: ASSIGN array, count, value
                    if (ir->operands.size() == 3 && std::dynamic_pointer_cast<IRArrayType>(dst->type)) {
                        // tCnt=t0, tVal=t1, tIdx=t2, tAddr=t3, baseReg=t4
                        auto tCnt  = Registers::t0();
                        auto tVal  = Registers::t1();
                        auto tIdx  = Registers::t2();
                        auto tAddr = Registers::t3();
                        auto baseR = Registers::t4();

                        // Load count and value
                        loadOp(ir->operands[1], tCnt, code);
                        loadOp(ir->operands[2], tVal, code);
                        // idx = 0
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, std::make_shared<Immediate>(0) });

                        // Compute baseReg for array
                        int baseOff = fi.varOffset[dst->getName()];
                        if (fi.paramArrayNames.count(dst->getName())) {
                            code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseR, std::make_shared<Address>(baseOff, Registers::fp()) });
                        } else {
                            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseR, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                        }

                        static int arrSetCounter = 0;
                        std::string Lloop = F.name + std::string("_arrset_") + std::to_string(arrSetCounter++);
                        std::string Lend  = F.name + std::string("_arrset_end_") + std::to_string(arrSetCounter++);

                        // loop label
                        code.emplace_back(MIPSOp::SLL, Lloop, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                        // if (idx >= cnt) goto end
                        code.emplace_back(MIPSOp::BGE, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, tCnt, std::make_shared<Label>(Lend) });
                        // addr = base + (idx<<2)
                        code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                        code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseR, tAddr });
                        // *(addr) = value
                        code.emplace_back(MIPSOp::SW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                        // idx++
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, tIdx, std::make_shared<Immediate>(1) });
                        // goto loop
                        code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(Lloop) });
                        // end label
                        code.emplace_back(MIPSOp::SLL, Lend, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                    } else {
                        // Simple scalar assign: ASSIGN dest, src
                        auto src = ir->operands[1];
                        auto t0 = Registers::t0();
                        loadOp(src, t0, code);
                        storeVar(dst->getName(), t0, code);
                    }
                    break;
                }
                case IRInstruction::OpCode::ADD:
                case IRInstruction::OpCode::SUB:
                case IRInstruction::OpCode::MULT:
                case IRInstruction::OpCode::DIV:
                case IRInstruction::OpCode::AND:
                case IRInstruction::OpCode::OR: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    auto t0 = Registers::t0();
                    auto t1 = Registers::t1();
                    auto t2 = Registers::t2();
                    loadOp(ir->operands[1], t0, code);
                    loadOp(ir->operands[2], t1, code);
                    MIPSOp op = MIPSOp::ADD;
                    if (ir->opCode == IRInstruction::OpCode::SUB) op = MIPSOp::SUB;
                    else if (ir->opCode == IRInstruction::OpCode::MULT) op = MIPSOp::MUL;
                    else if (ir->opCode == IRInstruction::OpCode::DIV) op = MIPSOp::DIV;
                    else if (ir->opCode == IRInstruction::OpCode::AND) op = MIPSOp::AND;
                    else if (ir->opCode == IRInstruction::OpCode::OR)  op = MIPSOp::OR;
                    code.emplace_back(op, "", std::vector<std::shared_ptr<MIPSOperand>>{ t2, t0, t1 });
                    storeVar(dst->getName(), t2, code);
                    break;
                }
                case IRInstruction::OpCode::GOTO: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        std::make_shared<Label>(qualLabel(F.name, lbl->getName()))
                    });
                    break;
                }
                case IRInstruction::OpCode::BREQ:
                case IRInstruction::OpCode::BRNEQ:
                case IRInstruction::OpCode::BRLT:
                case IRInstruction::OpCode::BRGT:
                case IRInstruction::OpCode::BRGEQ: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    auto t0 = Registers::t0();
                    auto t1 = Registers::t1();
                    loadOp(ir->operands[1], t0, code);
                    loadOp(ir->operands[2], t1, code);
                    MIPSOp bop = MIPSOp::BEQ;
                    if (ir->opCode == IRInstruction::OpCode::BRNEQ) bop = MIPSOp::BNE;
                    else if (ir->opCode == IRInstruction::OpCode::BRLT) bop = MIPSOp::BLT;
                    else if (ir->opCode == IRInstruction::OpCode::BRGT) bop = MIPSOp::BGT;
                    else if (ir->opCode == IRInstruction::OpCode::BRGEQ) bop = MIPSOp::BGE;
                    code.emplace_back(bop, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        t0, t1, std::make_shared<Label>(qualLabel(F.name, lbl->getName()))
                    });
                    break;
                }
                case IRInstruction::OpCode::CALL:
                case IRInstruction::OpCode::CALLR: {
                    size_t idx = (ir->opCode == IRInstruction::OpCode::CALLR) ? 2 : 1;
                    auto fnOp = std::dynamic_pointer_cast<IRFunctionOperand>(ir->operands[idx-1]);
                    std::string callee = fnOp ? fnOp->getName() : ir->operands[idx-1]->toString();
                    // syscalls
                    if (callee == "geti") {
                        out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                            Registers::v0(), std::make_shared<Immediate>(5)
                        });
                        out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            storeVar(dst->getName(), Registers::v0(), code);
                        }
                        break;
                    }
                    if (callee == "getc") {
                        // Read a character into $v0 using interpreter's char handler (v0=12)
                        out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                            Registers::v0(), std::make_shared<Immediate>(12)
                        });
                        out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            storeVar(dst->getName(), Registers::v0(), code);
                        }
                        break;
                    }
                    if (callee == "puti" || callee == "putc") {
                        // load arg0
                        auto t0 = Registers::t0();
                        if (idx < ir->operands.size()) loadOp(ir->operands[idx], t0, code);
                        code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::a0(), t0 });
                        int sc = (callee == "puti") ? 1 : 11;
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(sc) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        break;
                    }
                    if (callee == "putf") {
                        // load float arg into $f12 and syscall 2
                        auto f12 = std::make_shared<Register>(Register{"f12", true});
                        if (idx < ir->operands.size()) loadVarF32(ir->operands[idx], f12, code);
                        out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                            Registers::v0(), std::make_shared<Immediate>(2)
                        });
                        out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        break;
                    }
                    if (callee == "getf") {
                        // Read float into $f0 (syscall 6)
                        out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                            Registers::v0(), std::make_shared<Immediate>(6)
                        });
                        out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            auto f0 = std::make_shared<Register>(Register{"f0", true});
                            storeVarF32(dst->getName(), f0, code);
                        }
                        break;
                    }
                    // normal call: pass up to 4 args, push extras on stack
                    static std::shared_ptr<Register> aRegs[4] = { Registers::a0(), Registers::a1(), Registers::a2(), Registers::a3() };
                    for (size_t a = 0; a < 4 && idx + a < ir->operands.size(); ++a) {
                        auto arg = ir->operands[idx + a];
                        if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(arg)) {
                            if (std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                                int base = fi.varOffset[v->getName()];
                                if (fi.paramArrayNames.count(v->getName())) {
                                    // Array parameter: slot holds pointer; load it
                                    code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                                        aRegs[a], std::make_shared<Address>(base, Registers::fp())
                                    });
                                } else {
                                    // Local array: pass address of frame slot
                                    code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                                        aRegs[a], Registers::fp(), std::make_shared<Immediate>(base)
                                    });
                                }
                                continue;
                            }
                        }
                        auto t = Registers::t0();
                        loadOp(arg, t, code);
                        code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], t });
                    }
                    // Push extra args (right-to-left) so arg[4] is closest to callee frame
                    if (idx + 4 < ir->operands.size()) {
                        size_t extraStart = idx + 4; // first extra arg index
                        // collect extras in order, then push in reverse
                        std::vector<std::shared_ptr<IROperand>> extras;
                        for (size_t a = extraStart; a < ir->operands.size(); ++a) {
                            extras.push_back(ir->operands[a]);
                        }
                        for (size_t r = extras.size(); r-- > 0; ) {
                            auto t = Registers::t0();
                            loadOp(extras[r], t, code);
                            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                                Registers::sp(), Registers::sp(), std::make_shared<Immediate>(-4)
                            });
                            code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                                t, std::make_shared<Address>(0, Registers::sp())
                            });
                        }
                    }
                    code.emplace_back(MIPSOp::JAL, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        std::make_shared<Label>(callee)
                    });
                    // Pop extra args after call
                    if (idx + 4 < ir->operands.size()) {
                        int extra = int(ir->operands.size() - (idx + 4));
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                            Registers::sp(), Registers::sp(), std::make_shared<Immediate>(extra * 4)
                        });
                    }
                    if (ir->opCode == IRInstruction::OpCode::CALLR) {
                        auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                        storeVar(dst->getName(), Registers::v0(), code);
                    }
                    break;
                }
                case IRInstruction::OpCode::RETURN: {
                    auto t0 = Registers::t0();
                    loadOp(ir->operands[0], t0, code);
                    code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), t0 });
                    // jump to epilogue
                    code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        std::make_shared<Label>(F.name + std::string("_epilogue"))
                    });
                    break;
                }
                case IRInstruction::OpCode::ARRAY_STORE: {
                    // value, array, index
                    auto tVal = Registers::t0();
                    auto tIdx = Registers::t1();
                    auto tAddr = Registers::t2();
                    loadOp(ir->operands[0], tVal, code);
                    auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                    loadOp(ir->operands[2], tIdx, code);
                    // Compute address base: param-array -> load base pointer; local array -> $fp + baseOffset
                    std::shared_ptr<Register> baseReg = Registers::t3();
                    int baseOff = fi.varOffset[arrVar->getName()];
                    if (fi.paramArrayNames.count(arrVar->getName())) {
                        // load pointer from slot
                        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, std::make_shared<Address>(baseOff, Registers::fp()) });
                    } else {
                        // compute frame base address
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                    }
                    // tAddr = baseReg + (idx<<2)
                    code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                    code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseReg, tAddr });
                    code.emplace_back(MIPSOp::SW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                    break;
                }
                case IRInstruction::OpCode::ARRAY_LOAD: {
                    // dest, array, index
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    auto tIdx = Registers::t0();
                    auto tAddr = Registers::t1();
                    auto tVal = Registers::t2();
                    auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                    loadOp(ir->operands[2], tIdx, code);
                    // Compute base like in store
                    std::shared_ptr<Register> baseReg = Registers::t3();
                    int baseOff = fi.varOffset[arrVar->getName()];
                    if (fi.paramArrayNames.count(arrVar->getName())) {
                        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, std::make_shared<Address>(baseOff, Registers::fp()) });
                    } else {
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                    }
                    code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                    code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseReg, tAddr });
                    code.emplace_back(MIPSOp::LW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                    storeVar(dst->getName(), tVal, code);
                    break;
                }
                default: break;
            }
            out.insert(out.end(), code.begin(), code.end());
        }

        // Epilogue
        out.emplace_back(MIPSOp::SLL, F.name + std::string("_epilogue"), std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0)
        });
        out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::ra(), std::make_shared<Address>(0, Registers::fp())
        });
        out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::fp(), std::make_shared<Address>(4, Registers::fp())
        });
        out.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::sp(), Registers::sp(), std::make_shared<Immediate>(fi.frameBytes)
        });
        out.emplace_back(MIPSOp::JR, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::ra() });
    }
    return out;
}

// Legacy selector methods unused by the simple emitter.
std::vector<MIPSInstruction>
IRToMIPSSelector::selectFunction(const IRFunction& function) {
    (void)function;
    return {};
}

std::shared_ptr<Register>
InstructionSelector::getRegisterForOperand(std::shared_ptr<IROperand> irOp,
                                           SelectionContext& ctx) {
    if (!irOp) {
        // Fallback: give a scratch virtual
        return ctx.regManager.getVirtualRegister();
    }

    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(irOp)) {
        // Map IR variable name -> (possibly physical) register
        return ctx.regManager.getRegister(v->getName());
    }

    if (std::dynamic_pointer_cast<IRConstantOperand>(irOp)) {
        // For immediates we return a temp register; the *selector* should emit LI/ADDI
        // (handleImmediate returns a virtual scratch by policy)
        return ctx.regManager.handleImmediate(/*value_unused_here*/0);
    }

    // Labels/functions don’t have a GP register; return a scratch if someone calls us anyway.
    return ctx.regManager.getVirtualRegister();
}

std::vector<MIPSInstruction>
IRToMIPSSelector::selectInstruction(const IRInstruction& /*instruction*/, SelectionContext& /*ctx*/) {
            return {};
}

std::string
IRToMIPSSelector::generateAssembly(const std::vector<MIPSInstruction>& instructions) {
    std::ostringstream oss;
    // Ensure the text section is declared so the interpreter sets PC correctly
    oss << ".text\n";
    for (const auto& ins : instructions) {
        oss << ins.toString();
    }
    return oss.str();
}

void
IRToMIPSSelector::writeAssemblyFile(const std::string& filename,
                                    const std::vector<MIPSInstruction>& instructions) {
    const auto text = generateAssembly(instructions);

    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }
    ofs << text;
    if (!ofs.good()) {
        throw std::runtime_error("Failed to write assembly to: " + filename);
    }
}

} // namespace ircpp
