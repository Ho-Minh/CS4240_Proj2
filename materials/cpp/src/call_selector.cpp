#include "call_selector.hpp"

namespace ircpp {

std::vector<MIPSInstruction> CallSelector::select(const IRInstruction& ir, 
                                                  SelectionContext& ctx) {
    switch (ir.opCode) {
        case IRInstruction::OpCode::CALL:
            return selectCall(ir, ctx);
        case IRInstruction::OpCode::CALLR:
            return selectCallWithReturn(ir, ctx);
        case IRInstruction::OpCode::RETURN:
            return selectReturn(ir, ctx);
        default:
            return {};
    }
}

// ---------------------------------------------
// Helpers
// ---------------------------------------------

static std::string stripAt(const std::string& name) {
    return (!name.empty() && name[0] == '@') ? name.substr(1) : name;
}

static std::string funcNameFromOperand(const std::shared_ptr<IROperand>& op) {
    if (auto f = std::dynamic_pointer_cast<IRFunctionOperand>(op)) {
        return stripAt(f->getName());
    }
    return stripAt(op ? op->toString() : std::string());
}

static bool isImmediate(const std::shared_ptr<IROperand>& op) {
    return (bool)std::dynamic_pointer_cast<IRConstantOperand>(op);
}

static int toImmediateValue(const std::shared_ptr<IROperand>& op) {
    auto c = std::dynamic_pointer_cast<IRConstantOperand>(op);
    return c ? std::stoi(c->getValueString()) : 0;
}

// ---------------------------------------------
// CALL (no explicit return target)
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::selectCall(const IRInstruction& ir, 
                                                      SelectionContext& ctx) {
    std::vector<MIPSInstruction> code;
    if (ir.operands.empty()) return code;

    // Extract function name
    const std::string fname = funcNameFromOperand(ir.operands[0]);

    // Syscall shorthands
    if (fname == "geti" || fname == "puti" || fname == "putc") {
        std::vector<std::shared_ptr<IROperand>> params;
        for (size_t i = 1; i < ir.operands.size(); ++i) params.push_back(ir.operands[i]);
        auto sys = handleSystemCall(fname, params, ctx);
        code.insert(code.end(), sys.begin(), sys.end());
        return code;
    }

    // Save only currently allocated caller-saved registers
    {
        using namespace Registers;
        auto regs = ctx.regManager.getAllocatedCallerSavedRegs();
        for (auto& r : regs) {
            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                sp(), sp(), std::make_shared<Immediate>(-4)
            });
            code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                r, std::make_shared<Address>(0, sp())
            });
        }
    }

    // Setup parameters (also pushes extra args onto stack)
    int paramCount = (int)ir.operands.size() - 1;
    {
        std::vector<std::shared_ptr<IROperand>> params;
        for (size_t i = 1; i < ir.operands.size(); ++i) params.push_back(ir.operands[i]);
        auto ps = setupParameters(params, ctx);
        code.insert(code.end(), ps.begin(), ps.end());
    }

    // jal function
    {
        std::vector<std::shared_ptr<MIPSOperand>> ops;
        ops.push_back(std::make_shared<Label>(fname));
        code.emplace_back(MIPSOp::JAL, "", std::move(ops));
    }

    // Pop stack arguments (>4) after call
    {
        auto cleanup = manageCallStack(paramCount, ctx);
        code.insert(code.end(), cleanup.begin(), cleanup.end());
    }

    // Restore only those pushed
    {
        using namespace Registers;
        auto regs = ctx.regManager.getAllocatedCallerSavedRegs();
        for (auto it = regs.rbegin(); it != regs.rend(); ++it) {
            auto& r = *it;
            code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                r, std::make_shared<Address>(0, sp())
            });
            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                sp(), sp(), std::make_shared<Immediate>(4)
            });
        }
    }

    return code;
}

// ---------------------------------------------
// CALLR (dest = call(function, ...))
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::selectCallWithReturn(const IRInstruction& ir, 
                                                               SelectionContext& ctx) {
    std::vector<MIPSInstruction> code;
    if (ir.operands.size() < 2) return code;

    auto destVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[0]);
    const std::string fname = funcNameFromOperand(ir.operands[1]);

    // Syscall shorthands
    if (fname == "geti" || fname == "puti" || fname == "putc") {
        std::vector<std::shared_ptr<IROperand>> params;
        for (size_t i = 2; i < ir.operands.size(); ++i) params.push_back(ir.operands[i]);
        auto sys = handleSystemCall(fname, params, ctx);
        code.insert(code.end(), sys.begin(), sys.end());
        if (destVar) {
            auto destReg = ctx.regManager.getRegister(destVar->getName());
            std::vector<std::shared_ptr<MIPSOperand>> mops = { destReg, Registers::v0() };
            code.emplace_back(MIPSOp::MOVE, "", std::move(mops));
        }
        return code;
    }

    // Save caller-saved
    {
        auto pro = saveCallerSavedRegisters(ctx);
        code.insert(code.end(), pro.begin(), pro.end());
    }

    // Setup parameters
    int paramCount = (int)ir.operands.size() - 2;
    {
        std::vector<std::shared_ptr<IROperand>> params;
        for (size_t i = 2; i < ir.operands.size(); ++i) params.push_back(ir.operands[i]);
        auto ps = setupParameters(params, ctx);
        code.insert(code.end(), ps.begin(), ps.end());
    }

    // jal function
    {
        std::vector<std::shared_ptr<MIPSOperand>> ops;
        ops.push_back(std::make_shared<Label>(fname));
        code.emplace_back(MIPSOp::JAL, "", std::move(ops));
    }

    // Move return value
    if (destVar) {
        auto destReg = ctx.regManager.getRegister(destVar->getName());
        auto mv = handleReturnValue(destReg, ctx);
        code.insert(code.end(), mv.begin(), mv.end());
    }

    // Pop stack arguments (>4) after call
    {
        auto cleanup = manageCallStack(paramCount, ctx);
        code.insert(code.end(), cleanup.begin(), cleanup.end());
    }

    // Restore caller-saved
    {
        auto epi = restoreCallerSavedRegisters(ctx);
        code.insert(code.end(), epi.begin(), epi.end());
    }

    return code;
}

// ---------------------------------------------
// RETURN
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::selectReturn(const IRInstruction& ir, 
                                                        SelectionContext& ctx) {
    using namespace Registers;
    std::vector<MIPSInstruction> code;

    if (!ir.operands.empty()) {
        const auto& op0 = ir.operands[0];
        if (isImmediate(op0)) {
            int val = toImmediateValue(op0);
            code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                v0(), std::make_shared<Immediate>(val)
            });
        } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op0)) {
            auto src = ctx.regManager.getRegister(v->getName());
            code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{
                v0(), src
            });
        }
    }

    // Jump to function epilogue to restore callee-saved regs and stack before returning
    // The epilogue is labeled "<function>_epilogue" by IRToMIPSSelector::selectFunction
    if (!ctx.currentFunction.empty()) {
        code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{
            std::make_shared<Label>(ctx.currentFunction + std::string("_epilogue"))
        });
    } else {
        // Fallback: direct jr if function name is unknown
        code.emplace_back(MIPSOp::JR, "", std::vector<std::shared_ptr<MIPSOperand>>{
            Registers::ra()
        });
    }
    return code;
}

// ---------------------------------------------
// Parameter passing
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::setupParameters(
        const std::vector<std::shared_ptr<IROperand>>& params,
        SelectionContext& ctx) {
    using namespace Registers;

    std::vector<MIPSInstruction> code;

    // Helper: load immediate or variable into a target register
    auto loadIntoReg = [&](std::shared_ptr<Register> dst,
                           const std::shared_ptr<IROperand>& src) {
        if (isImmediate(src)) {
            int val = toImmediateValue(src);
            code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                dst, std::make_shared<Immediate>(val)
            });
        } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(src)) {
            auto r = ctx.regManager.getRegister(v->getName());
            code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{
                dst, r
            });
        }
    };

    const size_t n = params.size();

    // Push stack params (>4) right-to-left so lowest-indexed extra param is closest to return addr
    if (n > 4) {
        for (size_t i = n; i-- > 4; ) {
            std::shared_ptr<Register> srcReg;
            if (isImmediate(params[i])) {
                srcReg = ctx.regManager.getVirtualRegister();
                code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                    srcReg, std::make_shared<Immediate>(toImmediateValue(params[i]))
                });
            } else {
                auto v = std::dynamic_pointer_cast<IRVariableOperand>(params[i]);
                srcReg = v ? ctx.regManager.getRegister(v->getName())
                           : ctx.regManager.getVirtualRegister();
            }

            // addi $sp, $sp, -4
            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                sp(), sp(), std::make_shared<Immediate>(-4)
            });
            // sw src, 0($sp)
            code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                srcReg, std::make_shared<Address>(0, sp())
            });
        }
    }

    // First four in $a0..$a3
    static std::shared_ptr<Register> aRegs[4] = { a0(), a1(), a2(), a3() };
    for (size_t i = 0; i < std::min<size_t>(4, n); ++i) {
        loadIntoReg(aRegs[i], params[i]);
    }

    return code;
}

// ---------------------------------------------
// Return value
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::handleReturnValue(std::shared_ptr<Register> destReg,
                                                             SelectionContext& /*ctx*/) {
    std::vector<MIPSInstruction> code;
    code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{
        destReg, Registers::v0()
    });
    return code;
}

// ---------------------------------------------
// Save/restore caller-saved
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::saveCallerSavedRegisters(SelectionContext& ctx) {
    using namespace Registers;
    std::vector<MIPSInstruction> code;
    auto regs = ctx.regManager.getCallerSavedRegs();

    for (auto& r : regs) {
        // addi $sp, $sp, -4
        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            sp(), sp(), std::make_shared<Immediate>(-4)
        });
        // sw r, 0($sp)
        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            r, std::make_shared<Address>(0, sp())
        });
    }
    return code;
}

std::vector<MIPSInstruction> CallSelector::restoreCallerSavedRegisters(SelectionContext& ctx) {
    using namespace Registers;
    std::vector<MIPSInstruction> code;
    auto regs = ctx.regManager.getCallerSavedRegs();

    for (auto it = regs.rbegin(); it != regs.rend(); ++it) {
        auto& r = *it;
        // lw r, 0($sp)
        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            r, std::make_shared<Address>(0, sp())
        });
        // addi $sp, $sp, 4
        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            sp(), sp(), std::make_shared<Immediate>(4)
        });
    }
    return code;
}

// ---------------------------------------------
// Extra-arg stack cleanup
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::manageCallStack(int paramCount, SelectionContext& /*ctx*/) {
    using namespace Registers;
    std::vector<MIPSInstruction> code;
    int extra = std::max(0, paramCount - 4);
    if (extra > 0) {
        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            sp(), sp(), std::make_shared<Immediate>(extra * 4)
        });
    }
    return code;
}

// ---------------------------------------------
// Syscalls: geti, puti, putc
// ---------------------------------------------
std::vector<MIPSInstruction> CallSelector::handleSystemCall(const std::string& functionName,
                                                          const std::vector<std::shared_ptr<IROperand>>& params,
                                                          SelectionContext& ctx) {
    using namespace Registers;

    std::vector<MIPSInstruction> code;
    const std::string fn = stripAt(functionName);

    auto loadA0 = [&](const std::shared_ptr<IROperand>& src){
        if (!src) return;
        if (isImmediate(src)) {
            code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                a0(), std::make_shared<Immediate>(toImmediateValue(src))
            });
        } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(src)) {
            auto r = ctx.regManager.getRegister(v->getName());
            code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{
                a0(), r
            });
        }
    };

    if (fn == "geti") {
        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            v0(), std::make_shared<Immediate>(5)
        });
        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
        return code;
    }
    if (fn == "puti") {
        if (!params.empty()) loadA0(params[0]);
        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            v0(), std::make_shared<Immediate>(1)
        });
        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
        return code;
    }
    if (fn == "putc") {
        if (!params.empty()) loadA0(params[0]);
        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
            v0(), std::make_shared<Immediate>(11)
        });
        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
        return code;
    }

    // Unknown special — let the normal CALL path handle it.
    return code;
}

} // namespace ircpp
