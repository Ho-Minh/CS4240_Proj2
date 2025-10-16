#pragma once

#include "instruction_selector.hpp"

namespace ircpp {

// Selector for arithmetic IR instructions (ADD, SUB, MULT, DIV, AND, OR)
class ArithmeticSelector : public InstructionSelector {
public:
    std::vector<MIPSInstruction> select(const IRInstruction& ir, 
                                       SelectionContext& ctx) override;
    
private:
    // TODO: Implement addition selection
    // Convert IR ADD to MIPS ADD instruction
    std::vector<MIPSInstruction> selectAdd(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement subtraction selection
    // Convert IR SUB to MIPS SUB instruction
    std::vector<MIPSInstruction> selectSub(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement multiplication selection
    // Convert IR MULT to MIPS MUL + MFLO (multiplication uses HI/LO registers)
    std::vector<MIPSInstruction> selectMult(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement division selection
    // Convert IR DIV to MIPS DIV + MFLO (division uses HI/LO registers)
    std::vector<MIPSInstruction> selectDiv(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement logical AND selection
    // Convert IR AND to MIPS AND instruction
    std::vector<MIPSInstruction> selectAnd(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement logical OR selection
    // Convert IR OR to MIPS OR instruction
    std::vector<MIPSInstruction> selectOr(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement immediate optimization
    // Check if operands are immediate values and use immediate instructions when possible
    std::vector<MIPSInstruction> optimizeWithImmediate(MIPSOp opcode, 
                                                      std::shared_ptr<Register> dest,
                                                      std::shared_ptr<MIPSOperand> src1,
                                                      std::shared_ptr<MIPSOperand> src2,
                                                      SelectionContext& ctx);
};

} // namespace ircpp
