#include "arithmetic_selector.hpp"

namespace ircpp {

std::vector<MIPSInstruction> ArithmeticSelector::select(const IRInstruction& ir, 
                                                       SelectionContext& ctx) {
    // TODO: Implement arithmetic instruction selection
    // Dispatch to appropriate selector based on opcode
    
    switch (ir.opCode) {
        case IRInstruction::OpCode::ADD:
            return selectAdd(ir, ctx);
        case IRInstruction::OpCode::SUB:
            return selectSub(ir, ctx);
        case IRInstruction::OpCode::MULT:
            return selectMult(ir, ctx);
        case IRInstruction::OpCode::DIV:
            return selectDiv(ir, ctx);
        case IRInstruction::OpCode::AND:
            return selectAnd(ir, ctx);
        case IRInstruction::OpCode::OR:
            return selectOr(ir, ctx);
        default:
            return {}; // Should not reach here
    }
}

std::vector<MIPSInstruction> ArithmeticSelector::selectAdd(const IRInstruction& ir, 
                                                          SelectionContext& ctx) {
    // TODO: Implement ADD selection
    // IR: ADD dest, src1, src2
    // MIPS: add $dest, $src1, $src2
    // 
    // Steps:
    // 1. Get registers for operands
    // 2. Check if src2 is immediate (use addi)
    // 3. Generate appropriate MIPS instruction
    // 4. Return instruction(s)
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> ArithmeticSelector::selectSub(const IRInstruction& ir, 
                                                          SelectionContext& ctx) {
    // TODO: Implement SUB selection
    // IR: SUB dest, src1, src2
    // MIPS: sub $dest, $src1, $src2
    // 
    // Steps:
    // 1. Get registers for operands
    // 2. Check if src2 is immediate (use addi with negative)
    // 3. Generate MIPS instruction
    // 4. Return instruction(s)
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> ArithmeticSelector::selectMult(const IRInstruction& ir, 
                                                           SelectionContext& ctx) {
    // TODO: Implement MULT selection
    // IR: MULT dest, src1, src2
    // MIPS: mult $src1, $src2; mflo $dest
    // 
    // Steps:
    // 1. Get registers for src1 and src2
    // 2. Generate mult instruction
    // 3. Generate mflo instruction for result
    // 4. Return both instructions
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> ArithmeticSelector::selectDiv(const IRInstruction& ir, 
                                                          SelectionContext& ctx) {
    // TODO: Implement DIV selection
    // IR: DIV dest, src1, src2
    // MIPS: div $src1, $src2; mflo $dest
    // 
    // Steps:
    // 1. Get registers for src1 and src2
    // 2. Generate div instruction
    // 3. Generate mflo instruction for quotient
    // 4. Return both instructions
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> ArithmeticSelector::selectAnd(const IRInstruction& ir, 
                                                          SelectionContext& ctx) {
    // TODO: Implement AND selection
    // IR: AND dest, src1, src2
    // MIPS: and $dest, $src1, $src2
    // 
    // Steps:
    // 1. Get registers for operands
    // 2. Check if src2 is immediate (use andi)
    // 3. Generate appropriate MIPS instruction
    // 4. Return instruction(s)
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> ArithmeticSelector::selectOr(const IRInstruction& ir, 
                                                         SelectionContext& ctx) {
    // TODO: Implement OR selection
    // IR: OR dest, src1, src2
    // MIPS: or $dest, $src1, $src2
    // 
    // Steps:
    // 1. Get registers for operands
    // 2. Check if src2 is immediate (use ori)
    // 3. Generate appropriate MIPS instruction
    // 4. Return instruction(s)
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> ArithmeticSelector::optimizeWithImmediate(MIPSOp opcode, 
                                                                      std::shared_ptr<Register> dest,
                                                                      std::shared_ptr<MIPSOperand> src1,
                                                                      std::shared_ptr<MIPSOperand> src2,
                                                                      SelectionContext& ctx) {
    // TODO: Implement immediate optimization
    // Check if src2 is immediate and use immediate instruction
    // Examples:
    // - ADD with immediate -> ADDI
    // - AND with immediate -> ANDI
    // - OR with immediate -> ORI
    
    return {}; // Placeholder
}

} // namespace ircpp
