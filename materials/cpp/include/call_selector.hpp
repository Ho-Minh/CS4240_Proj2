#pragma once

#include "instruction_selector.hpp"

namespace ircpp {

// Selector for function call IR instructions (CALL, CALLR, RETURN)
class CallSelector : public InstructionSelector {
public:
    std::vector<MIPSInstruction> select(const IRInstruction& ir, 
                                       SelectionContext& ctx) override;
    
private:
    // TODO: Implement function call selection
    // Convert IR CALL to MIPS JAL instruction with parameter setup
    std::vector<MIPSInstruction> selectCall(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement function call with return value selection
    // Convert IR CALLR to MIPS JAL instruction with return value handling
    std::vector<MIPSInstruction> selectCallWithReturn(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement return selection
    // Convert IR RETURN to MIPS JR instruction with return value setup
    std::vector<MIPSInstruction> selectReturn(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement parameter passing
    // Setup function parameters in $a0-$a3 registers and stack
    std::vector<MIPSInstruction> setupParameters(const std::vector<std::shared_ptr<IROperand>>& params,
                                               SelectionContext& ctx);
    
    // TODO: Implement return value handling
    // Move return value from $v0 to target register
    std::vector<MIPSInstruction> handleReturnValue(std::shared_ptr<Register> destReg,
                                                 SelectionContext& ctx);
    
    // TODO: Implement caller-saved register preservation
    // Save caller-saved registers before function call
    std::vector<MIPSInstruction> saveCallerSavedRegisters(SelectionContext& ctx);
    
    // TODO: Implement caller-saved register restoration
    // Restore caller-saved registers after function call
    std::vector<MIPSInstruction> restoreCallerSavedRegisters(SelectionContext& ctx);
    
    // TODO: Implement stack management for function calls
    // Manage stack space for function parameters and return addresses
    std::vector<MIPSInstruction> manageCallStack(int paramCount, SelectionContext& ctx);
    
    // TODO: Implement system call handling
    // Handle special system calls (geti, puti, putc)
    std::vector<MIPSInstruction> handleSystemCall(const std::string& functionName,
                                                const std::vector<std::shared_ptr<IROperand>>& params,
                                                SelectionContext& ctx);
};

} // namespace ircpp
