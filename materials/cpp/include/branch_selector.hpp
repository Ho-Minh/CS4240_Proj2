#pragma once

#include "instruction_selector.hpp"

namespace ircpp {

// Selector for branch and control flow IR instructions (BREQ, BRNEQ, BRLT, BRGT, BRGEQ, GOTO)
class BranchSelector : public InstructionSelector {
public:
    std::vector<MIPSInstruction> select(const IRInstruction& ir, 
                                       SelectionContext& ctx) override;
    
private:
    // TODO: Implement branch equal selection
    // Convert IR BREQ to MIPS BEQ instruction
    std::vector<MIPSInstruction> selectBranchEqual(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement branch not equal selection
    // Convert IR BRNEQ to MIPS BNE instruction
    std::vector<MIPSInstruction> selectBranchNotEqual(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement branch less than selection
    // Convert IR BRLT to MIPS BLT instruction
    std::vector<MIPSInstruction> selectBranchLessThan(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement branch greater than selection
    // Convert IR BRGT to MIPS BGT instruction
    std::vector<MIPSInstruction> selectBranchGreaterThan(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement branch greater or equal selection
    // Convert IR BRGEQ to MIPS BGE instruction
    std::vector<MIPSInstruction> selectBranchGreaterEqual(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement unconditional jump selection
    // Convert IR GOTO to MIPS J instruction
    std::vector<MIPSInstruction> selectGoto(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement label selection
    // Convert IR LABEL to MIPS label (no instruction, just label)
    std::vector<MIPSInstruction> selectLabel(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement branch optimization
    // Optimize branches with immediate operands (use immediate compare instructions)
    std::vector<MIPSInstruction> optimizeBranchWithImmediate(MIPSOp opcode,
                                                           std::shared_ptr<Register> src1,
                                                           std::shared_ptr<MIPSOperand> src2,
                                                           const std::string& targetLabel,
                                                           SelectionContext& ctx);
};

} // namespace ircpp
