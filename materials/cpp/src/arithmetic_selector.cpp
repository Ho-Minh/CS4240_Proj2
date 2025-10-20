#include "arithmetic_selector.hpp"
#include "ir.hpp"
#include "mips_instructions.hpp"

namespace ircpp {

using std::shared_ptr;
using std::vector;

static inline bool isConst(const std::shared_ptr<IROperand>& op) {
    return std::dynamic_pointer_cast<IRConstantOperand>(op) != nullptr;
}

static inline int constToInt(const std::shared_ptr<IROperand>& op) {
    // IRConstantOperand::toString() yields the literal token; stoi handles +/- ints.
    auto c = std::static_pointer_cast<IRConstantOperand>(op);
    return std::stoi(c->toString());
}

vector<MIPSInstruction> ArithmeticSelector::select(const IRInstruction& ir, SelectionContext& ctx) {
    switch (ir.opCode) {
        case IRInstruction::OpCode::ADD:  return selectAdd(ir, ctx);
        case IRInstruction::OpCode::SUB:  return selectSub(ir, ctx);
        case IRInstruction::OpCode::MULT: return selectMult(ir, ctx);
        case IRInstruction::OpCode::DIV:  return selectDiv(ir, ctx);
        case IRInstruction::OpCode::AND:  return selectAnd(ir, ctx);
        case IRInstruction::OpCode::OR:   return selectOr(ir, ctx);
        default: return {};
    }
}

// --------------------------- ADD ---------------------------
vector<MIPSInstruction> ArithmeticSelector::selectAdd(const IRInstruction& ir, SelectionContext& ctx) {
    // IR: ADD dest, src1, src2
    auto dest = this->getRegisterForOperand(ir.operands[0], ctx); // Register
    auto src1 = this->getRegisterForOperand(ir.operands[1], ctx); // Register as MIPSOperand

    // If src2 is constant, build an Immediate; else, a register
    shared_ptr<MIPSOperand> src2Op;
    if (isConst(ir.operands[2])) {
        src2Op = std::make_shared<Immediate>(constToInt(ir.operands[2]));
    } else {
        src2Op = this->getRegisterForOperand(ir.operands[2], ctx);
    }

    // Let the helper choose ADDI when applicable
    return optimizeWithImmediate(MIPSOp::ADD, dest, src1, src2Op, ctx);
}

// --------------------------- SUB ---------------------------
vector<MIPSInstruction> ArithmeticSelector::selectSub(const IRInstruction& ir, SelectionContext& ctx) {
    // IR: SUB dest, src1, src2
    auto dest = this->getRegisterForOperand(ir.operands[0], ctx);
    auto src1 = this->getRegisterForOperand(ir.operands[1], ctx);

    shared_ptr<MIPSOperand> src2Op;
    if (isConst(ir.operands[2])) {
        src2Op = std::make_shared<Immediate>(constToInt(ir.operands[2]));
    } else {
        src2Op = this->getRegisterForOperand(ir.operands[2], ctx);
    }

    return optimizeWithImmediate(MIPSOp::SUB, dest, src1, src2Op, ctx);
}

// --------------------------- MULT ---------------------------
vector<MIPSInstruction> ArithmeticSelector::selectMult(const IRInstruction& ir, SelectionContext& ctx) {
    // IR: MULT dest, src1, src2
    // Your MIPSOp enum has MUL (3-operand form), so use that.
    auto dest = this->getRegisterForOperand(ir.operands[0], ctx);
    auto src1 = this->getRegisterForOperand(ir.operands[1], ctx);
    auto src2 = this->getRegisterForOperand(ir.operands[2], ctx);

    return {
        MIPSInstruction{
            MIPSOp::MUL, /*label*/"",
            std::vector<std::shared_ptr<MIPSOperand>>{ dest, src1, src2 }
        }
    };
}

// --------------------------- DIV ---------------------------
vector<MIPSInstruction> ArithmeticSelector::selectDiv(const IRInstruction& ir, SelectionContext& ctx) {
    // IR: DIV dest, src1, src2
    // Emit interpreter-supported 3-operand form: div $dest, $src1, $src2
    auto dest = this->getRegisterForOperand(ir.operands[0], ctx);
    auto src1 = this->getRegisterForOperand(ir.operands[1], ctx);
    auto src2 = this->getRegisterForOperand(ir.operands[2], ctx);
    return { MIPSInstruction{ MIPSOp::DIV, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ dest, src1, src2 } } };
}

// --------------------------- AND ---------------------------
vector<MIPSInstruction> ArithmeticSelector::selectAnd(const IRInstruction& ir, SelectionContext& ctx) {
    // IR: AND dest, src1, src2
    auto dest = this->getRegisterForOperand(ir.operands[0], ctx);
    auto src1 = this->getRegisterForOperand(ir.operands[1], ctx);

    shared_ptr<MIPSOperand> src2Op;
    if (isConst(ir.operands[2])) {
        src2Op = std::make_shared<Immediate>(constToInt(ir.operands[2]));
    } else {
        src2Op = this->getRegisterForOperand(ir.operands[2], ctx);
    }

    return optimizeWithImmediate(MIPSOp::AND, dest, src1, src2Op, ctx);
}

// --------------------------- OR ---------------------------
vector<MIPSInstruction> ArithmeticSelector::selectOr(const IRInstruction& ir, SelectionContext& ctx) {
    // IR: OR dest, src1, src2
    auto dest = this->getRegisterForOperand(ir.operands[0], ctx);
    auto src1 = this->getRegisterForOperand(ir.operands[1], ctx);

    shared_ptr<MIPSOperand> src2Op;
    if (isConst(ir.operands[2])) {
        src2Op = std::make_shared<Immediate>(constToInt(ir.operands[2]));
    } else {
        src2Op = this->getRegisterForOperand(ir.operands[2], ctx);
    }

    return optimizeWithImmediate(MIPSOp::OR, dest, src1, src2Op, ctx);
}

// ------------------ Immediate-aware emission ------------------
vector<MIPSInstruction>
ArithmeticSelector::optimizeWithImmediate(MIPSOp opcode,
                                          std::shared_ptr<Register> dest,
                                          std::shared_ptr<MIPSOperand> src1,
                                          std::shared_ptr<MIPSOperand> src2,
                                          SelectionContext& /*ctx*/) {
    // If src2 is an immediate, choose the immediate form where it exists.
    if (auto imm = std::dynamic_pointer_cast<Immediate>(src2)) {
        switch (opcode) {
            case MIPSOp::ADD:
                return { MIPSInstruction{ MIPSOp::ADDI, "", { dest, src1, imm } } };
            case MIPSOp::AND:
                return { MIPSInstruction{ MIPSOp::ANDI, "", { dest, src1, imm } } };
            case MIPSOp::OR:
                return { MIPSInstruction{ MIPSOp::ORI,  "", { dest, src1, imm } } };
            case MIPSOp::SUB: {
                // No SUBI in MIPS — emit ADDI with negative immediate.
                // Build a new Immediate with negated value.
                // Assuming Immediate exposes its value via toString() as well:
                int v = std::stoi(imm->toString());
                auto neg = std::make_shared<Immediate>(-v);
                return { MIPSInstruction{ MIPSOp::ADDI, "", { dest, src1, neg } } };
            }
            default:
                break; // fall through to non-immediate op
        }
    }

    // Fallback: emit the regular 3-operand instruction.
    return { MIPSInstruction{ opcode, "", { dest, src1, src2 } } };
}

} // namespace ircpp
