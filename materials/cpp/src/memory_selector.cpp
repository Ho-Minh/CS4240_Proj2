#include "memory_selector.hpp"

namespace ircpp {

std::vector<MIPSInstruction> MemorySelector::select(const IRInstruction& ir, 
                                                   SelectionContext& ctx) {
    // TODO: Implement memory instruction selection
    // Dispatch to appropriate selector based on opcode
    
    switch (ir.opCode) {
        case IRInstruction::OpCode::ASSIGN:
            return selectAssign(ir, ctx);
        case IRInstruction::OpCode::ARRAY_STORE:
            return selectArrayStore(ir, ctx);
        case IRInstruction::OpCode::ARRAY_LOAD:
            return selectArrayLoad(ir, ctx);
        default:
            return {}; // Should not reach here
    }
}

std::vector<MIPSInstruction> MemorySelector::selectAssign(const IRInstruction& ir, 
                                                        SelectionContext& ctx) {
    // TODO: Implement ASSIGN selection
    // IR: ASSIGN dest, src
    // MIPS: move $dest, $src OR li $dest, immediate
    // 
    // Steps:
    // 1. Get destination register
    // 2. Check if source is immediate or register
    // 3. Generate appropriate instruction (MOVE or LI)
    // 4. Return instruction
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> MemorySelector::selectArrayStore(const IRInstruction& ir, 
                                                            SelectionContext& ctx) {
    // TODO: Implement ARRAY_STORE selection
    // IR: ARRAY_STORE value, array, index
    // MIPS: sw $value, offset($array_base)
    // 
    // Steps:
    // 1. Get registers for value, array, and index
    // 2. Calculate array element address (array + index * element_size)
    // 3. Generate sw instruction
    // 4. Return instruction
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> MemorySelector::selectArrayLoad(const IRInstruction& ir, 
                                                           SelectionContext& ctx) {
    // TODO: Implement ARRAY_LOAD selection
    // IR: ARRAY_LOAD dest, array, index
    // MIPS: lw $dest, offset($array_base)
    // 
    // Steps:
    // 1. Get registers for dest, array, and index
    // 2. Calculate array element address (array + index * element_size)
    // 3. Generate lw instruction
    // 4. Return instruction
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> MemorySelector::calculateArrayAddress(std::shared_ptr<Register> resultReg,
                                                                 std::shared_ptr<Register> baseReg,
                                                                 std::shared_ptr<Register> indexReg,
                                                                 int elementSize,
                                                                 SelectionContext& ctx) {
    // TODO: Implement array address calculation
    // Calculate: resultReg = baseReg + indexReg * elementSize
    // 
    // Steps:
    // 1. If elementSize is power of 2, use SLL for multiplication
    // 2. Otherwise, use MULT instruction
    // 3. Add base address to get final address
    // 4. Return sequence of instructions
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> MemorySelector::selectImmediateAssign(std::shared_ptr<Register> destReg,
                                                                 int immediateValue,
                                                                 SelectionContext& ctx) {
    // TODO: Implement immediate assignment
    // MIPS: li $dest, immediate
    // 
    // Steps:
    // 1. Create immediate operand
    // 2. Generate li instruction
    // 3. Return instruction
    
    return {}; // Placeholder
}

std::vector<MIPSInstruction> MemorySelector::selectRegisterAssign(std::shared_ptr<Register> destReg,
                                                                std::shared_ptr<Register> srcReg,
                                                                SelectionContext& ctx) {
    // TODO: Implement register assignment
    // MIPS: move $dest, $src
    // 
    // Steps:
    // 1. Generate move instruction
    // 2. Return instruction
    
    return {}; // Placeholder
}

} // namespace ircpp
