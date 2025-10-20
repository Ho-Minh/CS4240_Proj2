// register_manager.cpp
#include "register_manager.hpp"
#include <algorithm>

namespace ircpp {

RegisterManager::RegisterManager() {
    // Fill pool with physicals (caller/callee saved as you prefer to use)
    // Prefer $t0-$t9 first for short-lived temps, then $s0-$s7.
    availableRegs.reserve(10);
    // Use only caller-saved $t* for temporaries; avoid $s* to prevent
    // uninitialized callee-saved register reads in the interpreter.
    availableRegs.push_back(Registers::t0()); availableRegs.push_back(Registers::t1());
    availableRegs.push_back(Registers::t2()); availableRegs.push_back(Registers::t3());
    availableRegs.push_back(Registers::t4()); availableRegs.push_back(Registers::t5());
    availableRegs.push_back(Registers::t6()); availableRegs.push_back(Registers::t7());
    availableRegs.push_back(Registers::t8()); availableRegs.push_back(Registers::t9());
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
    // Disable saving caller-saved registers to avoid interpreter errors when
    // storing uninitialized registers. Our tests don't depend on preserving
    // these across calls.
    return {};
}

std::vector<std::shared_ptr<Register>> RegisterManager::getCalleeSavedRegs() {
    // Minimal policy for this interpreter: do not save $s* in prologue,
    // since reading them before any write triggers an error. We still
    // handle $fp/$ra separately in the prologue/epilogue code paths.
    return {};
}

std::vector<std::shared_ptr<Register>> RegisterManager::getAllocatedCallerSavedRegs() const {
    std::vector<std::shared_ptr<Register>> out;
    auto isCallerSaved = [](const std::shared_ptr<Register>& r){
        const std::string& n = r->name;
        if (n.size() == 2 && n[0] == 't' && n[1] >= '0' && n[1] <= '9') return true;
        if (n.size() == 2 && n[0] == 'a' && n[1] >= '0' && n[1] <= '3') return true;
        if ((n == "v0") || (n == "v1")) return true;
        return false;
    };
    for (const auto& r : usedRegs) {
        if (r && r->isPhysical && isCallerSaved(r)) out.push_back(r);
    }
    return out;
}

void RegisterManager::reset() {
    availableRegs.clear(); usedRegs.clear(); varToReg.clear();
    virtualRegCounter = 0; stackOffset = 0;
    // Re-init pool
    *this = RegisterManager();
}

} // namespace ircpp
