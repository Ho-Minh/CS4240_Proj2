#include "instruction_selector.hpp"
#include "frame_builder.hpp"
#include "emit_helpers.hpp"
#include "alloc_naive.hpp"
#include "alloc_greedy.hpp"

#include <bits/stdc++.h>

namespace ircpp {

    IRToMIPSSelector::IRToMIPSSelector() : regManager(), registry(), mode(AllocMode::Naive) {}

    IRToMIPSSelector::IRToMIPSSelector(AllocMode m) : regManager(), registry(), mode(m) {}


// ==========================
// SelectionContext helpers
// ==========================

std::string SelectionContext::generateLabel(const std::string& irLabel) {
    // 1) If we already mapped this IR label, return the cached MIPS label
    auto it = labelMap.find(irLabel);
    if (it != labelMap.end()) return it->second;

    // 2) Otherwise create a unique label. Use function prefix if available.
    //    Example: fib.L3 or L3 as a fallback
    std::ostringstream oss;
    if (!currentFunction.empty()) {
        oss << currentFunction << "_L" << labelCounter++;
    } else {
        oss << "L" << labelCounter++;
    }
    std::string mipsLabel = oss.str();

    // 3) Store and return
    labelMap.emplace(irLabel, mipsLabel);
    return mipsLabel;
}

// Optional: if your header declares these inside SelectionContext,
// keep this exact signature. If your header uses a different place for them,
// move accordingly.

std::vector<MIPSInstruction>
SelectionContext::generateFunctionPrologue(const IRFunction& func) {
    using namespace Registers;

    // Decide callee-saved set (except fp/ra which we handle explicitly)
    auto callee = regManager.getCalleeSavedRegs();
    std::vector<std::shared_ptr<Register>> saveRegs;
    saveRegs.reserve(callee.size());
    for (auto& r : callee) {
        const auto n = r->toString();
        if (n != "$fp" && n != "$ra") saveRegs.push_back(r);
    }

    // Very simple local sizing: 4 bytes per declared local variable.
    // If you have real layout, plug it in here.
    int localBytes = 0;
    if (!func.variables.empty()) localBytes = static_cast<int>(func.variables.size()) * 4;

    // Frame size: ra (4) + fp (4) + callee-saved s-registers + locals
    const int calleeBytes = static_cast<int>(saveRegs.size()) * 4;
    const int frameBytes  = 4 /*ra*/ + 4 /*fp*/ + calleeBytes + localBytes;

    std::vector<MIPSInstruction> out;
    out.reserve(4 + (int)saveRegs.size());

    // Tag current function for label generation
    currentFunction = func.name;

    // Allocate frame: addi sp, sp, -frameBytes
    out.emplace_back(
        MIPSOp::ADDI,
        /*label*/ func.name, // put function label on the first instruction
        std::vector<std::shared_ptr<MIPSOperand>>{ sp(), sp(), std::make_shared<Immediate>(-frameBytes) }
    );

    // Save ra and fp
    out.emplace_back(MIPSOp::SW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ ra(), std::make_shared<Address>(0, sp()) });
    out.emplace_back(MIPSOp::SW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ fp(), std::make_shared<Address>(4, sp()) });

    // Establish frame pointer: move fp, sp
    out.emplace_back(MIPSOp::MOVE, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ fp(), sp() });

    // Save callee-saved s-registers at 8(sp) upward (currently none if policy returns empty)
    int off = 8;
    for (auto& r : saveRegs) {
        out.emplace_back(MIPSOp::SW, "",
            std::vector<std::shared_ptr<MIPSOperand>>{ r, std::make_shared<Address>(off, sp()) });
        off += 4;
    }

    // Initialize function parameters: move $a0-$a3 into their allocated variable registers
    if (!func.parameters.empty()) {
        static std::shared_ptr<Register> aRegs[4] = { a0(), a1(), a2(), a3() };
        const size_t n = std::min<size_t>(4, func.parameters.size());
        for (size_t i = 0; i < n; ++i) {
            auto& p = func.parameters[i];
            if (!p) continue;
            auto dst = regManager.getRegister(p->getName());
            out.emplace_back(MIPSOp::MOVE, "",
                std::vector<std::shared_ptr<MIPSOperand>>{ dst, aRegs[i] });
        }
        // Note: params beyond 4 would be on caller's stack; not needed for current tests
    }

    // Do not auto-initialize locals; rely on IR assignments to set values

    // No explicit locals init; space is reserved by sp adjustment already.
    return out;
}

std::vector<MIPSInstruction>
SelectionContext::generateFunctionEpilogue(const IRFunction& func) {
    using namespace Registers;

    // Must mirror the prologue computations to restore from correct slots.
    auto callee = regManager.getCalleeSavedRegs();
    std::vector<std::shared_ptr<Register>> saveRegs;
    saveRegs.reserve(callee.size());
    for (auto& r : callee) {
        const auto n = r->toString();
        if (n != "$fp" && n != "$ra") saveRegs.push_back(r);
    }
    const int calleeBytes = static_cast<int>(saveRegs.size()) * 4;
    int localBytes = 0;
    if (!func.variables.empty()) localBytes = static_cast<int>(func.variables.size()) * 4;
    const int frameBytes = 4 /*ra*/ + 4 /*fp*/ + calleeBytes + localBytes;

    std::vector<MIPSInstruction> out;
    out.reserve(5 + (int)saveRegs.size());

    // Restore s-registers (same offsets used in prologue), BEFORE fixing sp.
    int off = 8;
    for (auto& r : saveRegs) {
        out.emplace_back(MIPSOp::LW, "",
            std::vector<std::shared_ptr<MIPSOperand>>{ r, std::make_shared<Address>(off, sp()) });
        off += 4;
    }

    // Restore fp and ra
    out.emplace_back(MIPSOp::LW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ fp(), std::make_shared<Address>(4, sp()) });
    out.emplace_back(MIPSOp::LW, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ ra(), std::make_shared<Address>(0, sp()) });

    // Deallocate frame
    out.emplace_back(MIPSOp::ADDI, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ sp(), sp(), std::make_shared<Immediate>(frameBytes) });

    // Return
    out.emplace_back(MIPSOp::JR, "",
        std::vector<std::shared_ptr<MIPSOperand>>{ ra() });

    return out;
}

// ==========================
// IRToMIPSSelector
// ==========================

// Simple stack-frame based emitter: map every variable to a frame slot and
// always load operands into temporaries before operations. Avoids register
// liveness across calls/branches.

namespace {
}

std::vector<MIPSInstruction>
IRToMIPSSelector::selectProgram(const IRProgram& program) {
    std::vector<MIPSInstruction> out;
    // Program entry: call main and then exit (syscall 10)
    out.emplace_back(MIPSOp::JAL, "", std::vector<std::shared_ptr<MIPSOperand>>{
        std::make_shared<Label>(std::string("main"))
    });
    out.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::v0(), std::make_shared<Immediate>(10)
    });
    out.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});

    for (const auto& fn : program.functions) {
        if (!fn) continue;
        const IRFunction& F = *fn;

        if (getAllocMode() == AllocMode::Naive) {
            auto part = emitFunctionNaive(F);
            out.insert(out.end(), part.begin(), part.end());
        } else {
            auto part = emitFunctionGreedy(F);
            out.insert(out.end(), part.begin(), part.end());
        }
    }
    return out;
}

// Legacy selector methods unused by the simple emitter.
std::vector<MIPSInstruction>
IRToMIPSSelector::selectFunction(const IRFunction& function) {
    (void)function;
    return {};
}

std::shared_ptr<Register>
InstructionSelector::getRegisterForOperand(std::shared_ptr<IROperand> irOp,
                                           SelectionContext& ctx) {
    if (!irOp) {
        // Fallback: give a scratch virtual
        return ctx.regManager.getVirtualRegister();
    }

    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(irOp)) {
        // Map IR variable name -> (possibly physical) register
        return ctx.regManager.getRegister(v->getName());
    }

    if (std::dynamic_pointer_cast<IRConstantOperand>(irOp)) {
        // For immediates we return a temp register; the *selector* should emit LI/ADDI
        // (handleImmediate returns a virtual scratch by policy)
        return ctx.regManager.handleImmediate(/*value_unused_here*/0);
    }

    // Labels/functions don’t have a GP register; return a scratch if someone calls us anyway.
    return ctx.regManager.getVirtualRegister();
}

std::vector<MIPSInstruction>
IRToMIPSSelector::selectInstruction(const IRInstruction& /*instruction*/, SelectionContext& /*ctx*/) {
            return {};
}

std::string
IRToMIPSSelector::generateAssembly(const std::vector<MIPSInstruction>& instructions) {
    std::ostringstream oss;
    // Ensure the text section is declared so the interpreter sets PC correctly
    oss << ".text\n";
    for (const auto& ins : instructions) {
        oss << ins.toString();
    }
    return oss.str();
}

void
IRToMIPSSelector::writeAssemblyFile(const std::string& filename,
                                    const std::vector<MIPSInstruction>& instructions) {
    const auto text = generateAssembly(instructions);

    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }
    ofs << text;
    if (!ofs.good()) {
        throw std::runtime_error("Failed to write assembly to: " + filename);
    }
}

} // namespace ircpp
