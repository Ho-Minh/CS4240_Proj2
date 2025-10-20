#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "mips_instructions.hpp"

namespace ircpp {

// Manages register allocation and variable-to-register mapping
class RegisterManager {
private:
    // Available physical registers
    std::vector<std::shared_ptr<Register>> availableRegs;
    std::vector<std::shared_ptr<Register>> usedRegs;
    
    // Mapping from IR variables to allocated registers
    std::unordered_map<std::string, std::shared_ptr<Register>> varToReg;
    
    // Virtual register counter for spill registers
    int virtualRegCounter = 0;
    
    // Stack offset for spilled variables
    int stackOffset = 0;
    
public:
    RegisterManager();
    
    // TODO: Implement register allocation
    // Allocate a register for the given IR variable
    // Strategy: Use physical registers first, spill to stack if needed
    std::shared_ptr<Register> allocateRegister(const std::string& varName);
    
    // TODO: Implement register deallocation
    // Free a register when a variable goes out of scope
    void deallocateRegister(const std::string& varName);
    
    // TODO: Implement register lookup
    // Get the register allocated for a variable (or allocate if not found)
    std::shared_ptr<Register> getRegister(const std::string& varName);
    
    // TODO: Implement immediate handling
    // Handle immediate values (constants) - may need temporary registers
    std::shared_ptr<Register> handleImmediate(int value);
    
    // TODO: Implement spill management
    // Spill a register to stack and return the stack address
    std::shared_ptr<Address> spillRegister(std::shared_ptr<Register> reg);
    
    // TODO: Implement stack management
    // Allocate stack space for a variable
    int allocateStackSpace(int size = 4);
    
    // TODO: Implement register state tracking
    // Save/restore registers around function calls
    std::vector<std::shared_ptr<Register>> getCallerSavedRegs();
    std::vector<std::shared_ptr<Register>> getCalleeSavedRegs();

    // Return currently allocated caller-saved physical registers ($t0-$t9, $a0-$a3, $v0-$v1)
    std::vector<std::shared_ptr<Register>> getAllocatedCallerSavedRegs() const;
    
    // TODO: Implement cleanup
    // Reset manager state for new function
    void reset();
    
    // Getters
    int getStackOffset() const { return stackOffset; }
    std::shared_ptr<Register> getVirtualRegister();
};

} // namespace ircpp
