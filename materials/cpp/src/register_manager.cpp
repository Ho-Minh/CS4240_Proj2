// register_manager.cpp
#include "register_manager.hpp"
#include <algorithm>

namespace ircpp {

RegisterManager::RegisterManager() {
    // Fill pool with physicals (caller/callee saved as you prefer to use)
    // Prefer $t0-$t9 first for short-lived temps, then $s0-$s7.
    availableRegs.reserve(18);
    availableRegs.push_back(Registers::t0()); availableRegs.push_back(Registers::t1());
    availableRegs.push_back(Registers::t2()); availableRegs.push_back(Registers::t3());
    availableRegs.push_back(Registers::t4()); availableRegs.push_back(Registers::t5());
    availableRegs.push_back(Registers::t6()); availableRegs.push_back(Registers::t7());
    availableRegs.push_back(Registers::t8()); availableRegs.push_back(Registers::t9());
    availableRegs.push_back(Registers::s0()); availableRegs.push_back(Registers::s1());
    availableRegs.push_back(Registers::s2()); availableRegs.push_back(Registers::s3());
    availableRegs.push_back(Registers::s4()); availableRegs.push_back(Registers::s5());
    availableRegs.push_back(Registers::s6()); availableRegs.push_back(Registers::s7());
    virtualRegCounter = 0;
    stackOffset = 0;
}

std::shared_ptr<Register> RegisterManager::getVirtualRegister() {
    auto name = "vi" + std::to_string(virtualRegCounter++);
    return std::make_shared<Register>(Register{name, /*isPhysical=*/false});
}

std::shared_ptr<Register> RegisterManager::allocateRegister(const std::string& varName) {
    // Reuse existing mapping
    if (auto it = varToReg.find(varName); it != varToReg.end()) return it->second;

    std::shared_ptr<Register> reg;
    if (!availableRegs.empty()) {
        reg = availableRegs.back();
        availableRegs.pop_back();
        usedRegs.push_back(reg);
    } else {
        // No physicals left → use a virtual (caller will spill/load as needed)
        reg = getVirtualRegister();
        usedRegs.push_back(reg);
    }
    varToReg[varName] = reg;
    return reg;
}

void RegisterManager::deallocateRegister(const std::string& varName) {
    auto it = varToReg.find(varName);
    if (it == varToReg.end()) return;
    auto reg = it->second;

    // Return physicals to pool; virtuals just drop
    if (reg->isPhysical) {
        // erase from usedRegs
        auto it2 = std::find_if(usedRegs.begin(), usedRegs.end(),
                                [&](const std::shared_ptr<Register>& r){return r->name==reg->name;});
        if (it2 != usedRegs.end()) usedRegs.erase(it2);
        availableRegs.push_back(reg);
    }
    varToReg.erase(it);
}

std::shared_ptr<Register> RegisterManager::getRegister(const std::string& varName) {
    if (auto it = varToReg.find(varName); it != varToReg.end()) return it->second;
    return allocateRegister(varName);
}

// Allocator-only policy: just returns a temp reg; caller should emit `li $tmp, value`.
std::shared_ptr<Register> RegisterManager::handleImmediate(int /*value*/) {
    return getVirtualRegister();
}

// Allocates 4B by default, grows negative frame offsets relative to $fp.
// Returns the (negative) byte offset.
int RegisterManager::allocateStackSpace(int size) {
    // 8-byte align
    int aligned = (size + 7) & ~7;
    stackOffset += aligned;
    return -stackOffset; // e.g., -16 means -16($fp)
}

// Allocator-only policy: computes/returns spill address for reg; caller emits `sw/lw`.
std::shared_ptr<Address> RegisterManager::spillRegister(std::shared_ptr<Register> reg) {
    (void)reg; // the policy here doesn’t choose which reg to spill; caller decides.
    int off = allocateStackSpace(4);
    return std::make_shared<Address>(off, Registers::fp());
}

std::vector<std::shared_ptr<Register>> RegisterManager::getCallerSavedRegs() {
    return {
        Registers::t0(),Registers::t1(),Registers::t2(),Registers::t3(),Registers::t4(),
        Registers::t5(),Registers::t6(),Registers::t7(),Registers::t8(),Registers::t9(),
        Registers::a0(),Registers::a1(),Registers::a2(),Registers::a3(),
        Registers::v0(),Registers::v1()
    };
}

std::vector<std::shared_ptr<Register>> RegisterManager::getCalleeSavedRegs() {
    return {
        Registers::s0(),Registers::s1(),Registers::s2(),Registers::s3(),
        Registers::s4(),Registers::s5(),Registers::s6(),Registers::s7(),
        Registers::fp(),Registers::ra()
    };
}

void RegisterManager::reset() {
    availableRegs.clear(); usedRegs.clear(); varToReg.clear();
    virtualRegCounter = 0; stackOffset = 0;
    // Re-init pool
    *this = RegisterManager();
}

} // namespace ircpp
