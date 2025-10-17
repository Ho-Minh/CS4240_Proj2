#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include "ir.hpp"
#include "mips_instructions.hpp"
#include "register_manager.hpp"

namespace ircpp {

// Forward declarations
class InstructionSelector;
class SelectionContext;

// Context for instruction selection containing register manager, labels, etc.
struct SelectionContext {
    RegisterManager& regManager;
    std::unordered_map<std::string, std::string> labelMap;  // IR label -> MIPS label
    int labelCounter = 0;
    std::string currentFunction;
    int stackFrameSize = 0;
    
    SelectionContext(RegisterManager& rm) : regManager(rm) {}
    
    // TODO: Implement label generation
    // Generate unique MIPS label from IR label
    std::string generateLabel(const std::string& irLabel);
    
    // TODO: Implement function setup
    // Setup function prologue (save registers, allocate stack)
    std::vector<MIPSInstruction> generateFunctionPrologue(const IRFunction& func);
    
    // TODO: Implement function cleanup
    // Setup function epilogue (restore registers, deallocate stack)
    std::vector<MIPSInstruction> generateFunctionEpilogue(const IRFunction& func);
};

// Base class for instruction selection strategies
class InstructionSelector {
public:
    virtual ~InstructionSelector() = default;
    
    // TODO: Implement instruction selection
    // Select MIPS instructions for the given IR instruction
    virtual std::vector<MIPSInstruction> select(const IRInstruction& ir, 
                                               SelectionContext& ctx) = 0;
    
    // TODO: Implement operand handling
    // Convert IR operand to MIPS operand (register/immediate)
    std::shared_ptr<MIPSOperand> convertOperand(std::shared_ptr<IROperand> irOp, 
                                               SelectionContext& ctx);
    
    // TODO: Implement register allocation for operands
    // Get or allocate register for IR operand
    std::shared_ptr<Register> getRegisterForOperand(std::shared_ptr<IROperand> irOp, 
                                                   SelectionContext& ctx);
};

// Registry for mapping IR opcodes to their selectors
class SelectorRegistry {
private:
    std::unordered_map<IRInstruction::OpCode, std::unique_ptr<InstructionSelector>> selectors;
    
public:
    SelectorRegistry() = default;
    ~SelectorRegistry() = default;
    
    // TODO: Implement selector registration
    // Register a selector for an IR opcode
    void registerSelector(IRInstruction::OpCode opcode, 
                         std::unique_ptr<InstructionSelector> selector);
    
    // TODO: Implement instruction selection dispatch
    // Select MIPS instructions for IR instruction using appropriate selector
    std::vector<MIPSInstruction> select(const IRInstruction& ir, SelectionContext& ctx);
};

// Main instruction selector that coordinates the selection process
class IRToMIPSSelector {
private:
    SelectorRegistry registry;
    RegisterManager regManager;
    
public:
    IRToMIPSSelector();
    
    // TODO: Implement program selection
    // Convert entire IR program to MIPS assembly
    std::vector<MIPSInstruction> selectProgram(const IRProgram& program);
    
    // TODO: Implement function selection
    // Convert IR function to MIPS assembly
    std::vector<MIPSInstruction> selectFunction(const IRFunction& function);
    
    // TODO: Implement instruction-by-instruction selection
    // Select MIPS instructions for a single IR instruction
    std::vector<MIPSInstruction> selectInstruction(const IRInstruction& instruction, 
                                                  SelectionContext& ctx);
    
    // TODO: Implement assembly generation
    // Generate MIPS assembly text from instructions
    std::string generateAssembly(const std::vector<MIPSInstruction>& instructions);
    
    // TODO: Implement file output
    // Write MIPS assembly to file
    void writeAssemblyFile(const std::string& filename, 
                          const std::vector<MIPSInstruction>& instructions);
};

} // namespace ircpp
