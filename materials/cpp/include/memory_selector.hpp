#pragma once

#include "instruction_selector.hpp"

namespace ircpp {

// Selector for memory access IR instructions (ASSIGN, ARRAY_STORE, ARRAY_LOAD)
class MemorySelector : public InstructionSelector {
public:
    std::vector<MIPSInstruction> select(const IRInstruction& ir, 
                                       SelectionContext& ctx) override;
    
private:
    // TODO: Implement assignment selection
    // Convert IR ASSIGN to MIPS MOVE or LI instruction
    std::vector<MIPSInstruction> selectAssign(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement array store selection
    // Convert IR ARRAY_STORE to MIPS SW instruction
    std::vector<MIPSInstruction> selectArrayStore(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement array load selection
    // Convert IR ARRAY_LOAD to MIPS LW instruction
    std::vector<MIPSInstruction> selectArrayLoad(const IRInstruction& ir, SelectionContext& ctx);
    
    // TODO: Implement array indexing calculation
    // Calculate array element address (base + index * element_size)
    std::vector<MIPSInstruction> calculateArrayAddress(std::shared_ptr<Register> resultReg,
                                                     std::shared_ptr<Register> baseReg,
                                                     std::shared_ptr<Register> indexReg,
                                                     int elementSize,
                                                     SelectionContext& ctx);
    
    // TODO: Implement immediate assignment optimization
    // Use LI instruction for immediate value assignments
    std::vector<MIPSInstruction> selectImmediateAssign(std::shared_ptr<Register> destReg,
                                                     int immediateValue,
                                                     SelectionContext& ctx);
    
    // TODO: Implement register-to-register assignment
    // Use MOVE instruction for register assignments
    std::vector<MIPSInstruction> selectRegisterAssign(std::shared_ptr<Register> destReg,
                                                    std::shared_ptr<Register> srcReg,
                                                    SelectionContext& ctx);
};

} // namespace ircpp
