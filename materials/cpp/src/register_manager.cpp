#include "register_manager.hpp"

namespace ircpp {

RegisterManager::RegisterManager() {
    // TODO: Initialize available physical registers
    // Initialize availableRegs with MIPS physical registers
    // Suggested order: $t0-$t9, $s0-$s7, $a0-$a3
    // Reserve $v0, $v1, $sp, $fp, $ra for special purposes
}

std::shared_ptr<Register> RegisterManager::allocateRegister(const std::string& varName) {
    // TODO: Implement register allocation
    // Strategy:
    // 1. Check if variable already has a register assigned
    // 2. If available physical registers exist, allocate one
    // 3. If no physical registers available, spill a register to stack
    // 4. Update varToReg mapping
    // 5. Return allocated register
    
    return nullptr; // Placeholder
}

void RegisterManager::deallocateRegister(const std::string& varName) {
    // TODO: Implement register deallocation
    // 1. Find register assigned to variable in varToReg
    // 2. Move register back to availableRegs
    // 3. Remove from varToReg mapping
    // 4. Update usedRegs list
}

std::shared_ptr<Register> RegisterManager::getRegister(const std::string& varName) {
    // TODO: Implement register lookup
    // 1. Check varToReg mapping first
    // 2. If not found, call allocateRegister
    // 3. Return the register
    
    return nullptr; // Placeholder
}

std::shared_ptr<Register> RegisterManager::handleImmediate(int value) {
    // TODO: Implement immediate handling
    // For small immediate values, use LI instruction
    // For large values, may need multiple instructions
    // Strategy:
    // 1. Allocate temporary register
    // 2. Use LI instruction for immediate loading
    // 3. Return temporary register
    
    return nullptr; // Placeholder
}

std::shared_ptr<Address> RegisterManager::spillRegister(std::shared_ptr<Register> reg) {
    // TODO: Implement register spilling
    // 1. Allocate stack space
    // 2. Generate SW instruction to save register to stack
    // 3. Return stack address for later restoration
    
    return nullptr; // Placeholder
}

int RegisterManager::allocateStackSpace(int size) {
    // TODO: Implement stack space allocation
    // 1. Update stackOffset by size
    // 2. Return offset for stack allocation
    // 3. Handle alignment (MIPS requires 4-byte alignment)
    
    return stackOffset; // Placeholder
}

std::vector<std::shared_ptr<Register>> RegisterManager::getCallerSavedRegs() {
    // TODO: Return caller-saved registers
    // MIPS caller-saved: $t0-$t9, $a0-$a3, $v0-$v1
    return {};
}

std::vector<std::shared_ptr<Register>> RegisterManager::getCalleeSavedRegs() {
    // TODO: Return callee-saved registers
    // MIPS callee-saved: $s0-$s7, $fp, $ra
    return {};
}

void RegisterManager::reset() {
    // TODO: Reset manager state
    // 1. Clear varToReg mapping
    // 2. Move all used registers back to available
    // 3. Reset virtual register counter
    // 4. Reset stack offset
}

std::shared_ptr<Register> RegisterManager::getVirtualRegister() {
    // TODO: Create virtual register
    // 1. Generate unique virtual register name
    // 2. Create register with isPhysical = false
    // 3. Increment virtual register counter
    // 4. Return virtual register
    
    return nullptr; // Placeholder
}

} // namespace ircpp
