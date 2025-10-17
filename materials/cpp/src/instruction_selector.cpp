#include "instruction_selector.hpp"
#include "arithmetic_selector.hpp"
#include "branch_selector.hpp"
#include "memory_selector.hpp"
#include "call_selector.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ircpp {

    IRToMIPSSelector::IRToMIPSSelector() : regManager(), registry() {}


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
        oss << currentFunction << ".L" << labelCounter++;
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

    // Save callee-saved s-registers at 8(sp) upward
    int off = 8;
    for (auto& r : saveRegs) {
        out.emplace_back(MIPSOp::SW, "",
            std::vector<std::shared_ptr<MIPSOperand>>{ r, std::make_shared<Address>(off, sp()) });
        off += 4;
    }

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

std::vector<MIPSInstruction>
IRToMIPSSelector::selectProgram(const IRProgram& program) {
    std::vector<MIPSInstruction> out;
    SelectionContext ctx{ regManager /*passed by ref in ctor or member*/ };

    for (const auto& fn : program.functions) {
        if (!fn) continue;
        auto body = selectFunction(*fn);
        out.insert(out.end(), body.begin(), body.end());
    }
    return out;
}

std::vector<MIPSInstruction>
IRToMIPSSelector::selectFunction(const IRFunction& function) {
    SelectionContext ctx{ regManager /*passed by ref in ctor or member*/ };
    // Reset per-function state
    ctx.regManager.reset();
    ctx.currentFunction = function.name;

    std::vector<MIPSInstruction> out;

    // Prologue
    {
        auto pro = ctx.generateFunctionPrologue(function);
        out.insert(out.end(), pro.begin(), pro.end());
    }

    // Body
    for (const auto& ir : function.instructions) {
        if (!ir) continue;
        auto part = selectInstruction(*ir, ctx);
        out.insert(out.end(), part.begin(), part.end());
    }

    // Epilogue (simple layout: returns may early-exit; that’s fine)
    {
        auto epi = ctx.generateFunctionEpilogue(function);
        out.insert(out.end(), epi.begin(), epi.end());
    }

    return out;
}

std::vector<MIPSInstruction>
IRToMIPSSelector::selectInstruction(const IRInstruction& instruction, SelectionContext& ctx) {
    // Use dedicated sub-selectors to keep this dispatcher lean
    static ArithmeticSelector arithSel;
    static MemorySelector     memSel;
    static BranchSelector     brSel;
    static CallSelector       callSel;

    // Group opcodes roughly by domain, then delegate
    switch (instruction.opCode) {
        // Arithmetic / logic
        case IRInstruction::OpCode::ADD:
        case IRInstruction::OpCode::SUB:
        case IRInstruction::OpCode::MULT:
        case IRInstruction::OpCode::DIV:
        case IRInstruction::OpCode::AND:
        case IRInstruction::OpCode::OR:
            return arithSel.select(instruction, ctx);

        // Memory / assignment
        case IRInstruction::OpCode::ASSIGN:
        case IRInstruction::OpCode::ARRAY_STORE:
        case IRInstruction::OpCode::ARRAY_LOAD:
            return memSel.select(instruction, ctx);

        // Branching & labels
        case IRInstruction::OpCode::BREQ:
        case IRInstruction::OpCode::BRNEQ:
        case IRInstruction::OpCode::BRLT:
        case IRInstruction::OpCode::BRGT:
        case IRInstruction::OpCode::BRGEQ:
        case IRInstruction::OpCode::GOTO:
        case IRInstruction::OpCode::LABEL:
            return brSel.select(instruction, ctx);

        // Calls / returns
        case IRInstruction::OpCode::CALL:
        case IRInstruction::OpCode::CALLR:
        case IRInstruction::OpCode::RETURN:
            return callSel.select(instruction, ctx);

        default:
            // Not recognized / NOP for now
            return {};
    }
}

std::string
IRToMIPSSelector::generateAssembly(const std::vector<MIPSInstruction>& instructions) {
    std::ostringstream oss;
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
