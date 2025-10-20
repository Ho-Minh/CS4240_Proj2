#include "alloc_naive.hpp"
#include "frame_builder.hpp"
#include "emit_helpers.hpp"
#include <bits/stdc++.h>

namespace ircpp {

std::vector<MIPSInstruction> emitFunctionNaive(const IRFunction& F) {
    std::vector<MIPSInstruction> out;
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

    // Store first 4 parameters to their slots
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
    // Load stack-passed parameters
    if (F.parameters.size() > 4) {
        for (size_t i = 4; i < F.parameters.size(); ++i) {
            auto p = F.parameters[i]; if (!p) continue;
            int varOff = fi.varOffset[p->getName()];
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

    auto loadOp = [&](std::shared_ptr<IROperand> op, std::shared_ptr<Register> dst,
                      std::vector<MIPSInstruction>& code){
        if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(op)) {
            int val = std::stoi(c->getValueString());
            code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                dst, std::make_shared<Immediate>(val)
            });
        } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
            int off = fi.varOffset[v->getName()];
            code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                dst, std::make_shared<Address>(off, Registers::fp())
            });
        }
    };

    auto storeVar = [&](const std::string& name, std::shared_ptr<Register> src,
                        std::vector<MIPSInstruction>& code){
        int off = fi.varOffset[name];
        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            src, std::make_shared<Address>(off, Registers::fp())
        });
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
                if (ir->operands.size() == 3 && std::dynamic_pointer_cast<IRArrayType>(dst->type)) {
                    auto tCnt  = Registers::t0();
                    auto tVal  = Registers::t1();
                    auto tIdx  = Registers::t2();
                    auto tAddr = Registers::t3();
                    auto baseR = Registers::t4();
                    loadOp(ir->operands[1], tCnt, code);
                    loadOp(ir->operands[2], tVal, code);
                    code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, std::make_shared<Immediate>(0) });
                    int baseOff = fi.varOffset[dst->getName()];
                    if (fi.paramArrayNames.count(dst->getName())) {
                        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseR, std::make_shared<Address>(baseOff, Registers::fp()) });
                    } else {
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseR, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                    }
                    static int arrSetCounter = 0;
                    std::string Lloop = F.name + std::string("_arrset_") + std::to_string(arrSetCounter++);
                    std::string Lend  = F.name + std::string("_arrset_end_") + std::to_string(arrSetCounter++);
                    code.emplace_back(MIPSOp::SLL, Lloop, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                    code.emplace_back(MIPSOp::BGE, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, tCnt, std::make_shared<Label>(Lend) });
                    code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                    code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseR, tAddr });
                    code.emplace_back(MIPSOp::SW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                    code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, tIdx, std::make_shared<Immediate>(1) });
                    code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(Lloop) });
                    code.emplace_back(MIPSOp::SLL, Lend, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                } else {
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
                    auto t0 = Registers::t0();
                    if (idx < ir->operands.size()) loadOp(ir->operands[idx], t0, code);
                    code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::a0(), t0 });
                    int sc = (callee == "puti") ? 1 : 11;
                    code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(sc) });
                    code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                    break;
                }
                if (callee == "putf") {
                    auto f12 = std::make_shared<Register>(Register{"f12", true});
                    if (idx < ir->operands.size()) {
                        if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[idx])) {
                            int off = fi.varOffset[v->getName()];
                            code.emplace_back(MIPSOp::L_S, "", std::vector<std::shared_ptr<MIPSOperand>>{
                                f12, std::make_shared<Address>(off, Registers::fp())
                            });
                        }
                    }
                    out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        Registers::v0(), std::make_shared<Immediate>(2)
                    });
                    out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                    break;
                }
                if (callee == "getf") {
                    out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                        Registers::v0(), std::make_shared<Immediate>(6)
                    });
                    out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                    if (ir->opCode == IRInstruction::OpCode::CALLR) {
                        auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                        auto f0 = std::make_shared<Register>(Register{"f0", true});
                        // store float to slot
                        int off = fi.varOffset[dst->getName()];
                        code.emplace_back(MIPSOp::S_S, "", std::vector<std::shared_ptr<MIPSOperand>>{ f0, std::make_shared<Address>(off, Registers::fp()) });
                    }
                    break;
                }
                static std::shared_ptr<Register> aRegs[4] = { Registers::a0(), Registers::a1(), Registers::a2(), Registers::a3() };
                for (size_t a = 0; a < 4 && idx + a < ir->operands.size(); ++a) {
                    auto arg = ir->operands[idx + a];
                    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(arg)) {
                        if (std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                            int base = fi.varOffset[v->getName()];
                            if (fi.paramArrayNames.count(v->getName())) {
                                code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], std::make_shared<Address>(base, Registers::fp()) });
                            } else {
                                code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], Registers::fp(), std::make_shared<Immediate>(base) });
                            }
                            continue;
                        }
                    }
                    auto t = Registers::t0();
                    loadOp(arg, t, code);
                    code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], t });
                }
                if (idx + 4 < ir->operands.size()) {
                    size_t extraStart = idx + 4;
                    std::vector<std::shared_ptr<IROperand>> extras;
                    for (size_t a = extraStart; a < ir->operands.size(); ++a) extras.push_back(ir->operands[a]);
                    for (size_t r = extras.size(); r-- > 0; ) {
                        auto t = Registers::t0();
                        loadOp(extras[r], t, code);
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::sp(), Registers::sp(), std::make_shared<Immediate>(-4) });
                        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ t, std::make_shared<Address>(0, Registers::sp()) });
                    }
                }
                code.emplace_back(MIPSOp::JAL, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(callee) });
                if (idx + 4 < ir->operands.size()) {
                    int extra = int(ir->operands.size() - (idx + 4));
                    code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::sp(), Registers::sp(), std::make_shared<Immediate>(extra * 4) });
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
                code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(F.name + std::string("_epilogue")) });
                break;
            }
            case IRInstruction::OpCode::ARRAY_STORE: {
                auto tVal = Registers::t0();
                auto tIdx = Registers::t1();
                auto tAddr = Registers::t2();
                loadOp(ir->operands[0], tVal, code);
                auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                loadOp(ir->operands[2], tIdx, code);
                std::shared_ptr<Register> baseReg = Registers::t3();
                int baseOff = fi.varOffset[arrVar->getName()];
                if (fi.paramArrayNames.count(arrVar->getName())) {
                    code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, std::make_shared<Address>(baseOff, Registers::fp()) });
                } else {
                    code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                }
                code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseReg, tAddr });
                code.emplace_back(MIPSOp::SW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                break;
            }
            case IRInstruction::OpCode::ARRAY_LOAD: {
                auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                auto tIdx = Registers::t0();
                auto tAddr = Registers::t1();
                auto tVal = Registers::t2();
                auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                loadOp(ir->operands[2], tIdx, code);
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

    return out;
}

}


