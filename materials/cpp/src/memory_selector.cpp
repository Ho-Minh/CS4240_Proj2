#include "memory_selector.hpp"
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>

namespace ircpp {

std::vector<MIPSInstruction> MemorySelector::select(const IRInstruction& ir, 
                                                   SelectionContext& ctx) {
    // Dispatch to appropriate selector based on opcode
    switch (ir.opCode) {
        case IRInstruction::OpCode::ASSIGN:
            return selectAssign(ir, ctx);
        case IRInstruction::OpCode::ARRAY_STORE:
            return selectArrayStore(ir, ctx);
        case IRInstruction::OpCode::ARRAY_LOAD:
            return selectArrayLoad(ir, ctx);
        default:
            return {}; // Unsupported in this selector
    }
}

std::vector<MIPSInstruction> MemorySelector::selectAssign(const IRInstruction& ir, 
                                                        SelectionContext& ctx) {
    // IR: ASSIGN dest, src
    // dest must be a variable; src may be variable or constant
    std::vector<MIPSInstruction> out;

    if (ir.operands.size() < 2) return out;

    // Destination register
    auto dstVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[0]);
    if (!dstVar) return out; // defensive
    auto dst = ctx.regManager.getRegister(dstVar->getName());

    // Source
    if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(ir.operands[1])) {
        int imm = std::atoi(c->toString().c_str());
        auto seq = selectImmediateAssign(dst, imm, ctx);
        out.insert(out.end(), seq.begin(), seq.end());
    } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[1])) {
        auto src = ctx.regManager.getRegister(v->getName());
        auto seq = selectRegisterAssign(dst, src, ctx);
        out.insert(out.end(), seq.begin(), seq.end());
    }
    return out;
}

std::vector<MIPSInstruction> MemorySelector::selectArrayStore(const IRInstruction& ir, 
                                                            SelectionContext& ctx) {
    // IR (by spec): ARRAY_STORE value, array, index
    std::vector<MIPSInstruction> out;
    if (ir.operands.size() < 3) return out;

    // Value register
    std::shared_ptr<Register> valueReg;
    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[0])) {
        valueReg = ctx.regManager.getRegister(v->getName());
    } else if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(ir.operands[0])) {
        // load immediate into temp then store
        valueReg = ctx.regManager.getVirtualRegister();
        auto liSeq = selectImmediateAssign(valueReg, std::atoi(c->toString().c_str()), ctx);
        out.insert(out.end(), liSeq.begin(), liSeq.end());
    } else {
        return out;
    }

    // Array base register (treat array variable as base address)
    auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[1]);
    if (!arrVar) return out;
    auto base = ctx.regManager.getRegister(arrVar->getName());

    // Index could be immediate or variable
    if (auto idxConst = std::dynamic_pointer_cast<IRConstantOperand>(ir.operands[2])) {
        int idx = std::atoi(idxConst->toString().c_str());
        int elementSize = 4; // int/float 4B in our IR
        int offset = idx * elementSize;
        MIPSInstruction sw;
        sw.op = MIPSOp::SW;
        sw.operands = { valueReg, std::make_shared<Address>(offset, base) };
        out.push_back(std::move(sw));
    } else if (auto idxVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[2])) {
        auto idxReg = ctx.regManager.getRegister(idxVar->getName());
        // compute effective address into a temp
        auto addrReg = ctx.regManager.getVirtualRegister();
        auto calc = calculateArrayAddress(addrReg, base, idxReg, /*elementSize=*/4, ctx);
        out.insert(out.end(), calc.begin(), calc.end());
        MIPSInstruction sw;
        sw.op = MIPSOp::SW;
        sw.operands = { valueReg, std::make_shared<Address>(0, addrReg) };
        out.push_back(std::move(sw));
    }
    return out;
}

std::vector<MIPSInstruction> MemorySelector::selectArrayLoad(const IRInstruction& ir, 
                                                           SelectionContext& ctx) {
    // IR: ARRAY_LOAD dest, array, index
    std::vector<MIPSInstruction> out;
    if (ir.operands.size() < 3) return out;

    // Destination register
    auto dstVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[0]);
    if (!dstVar) return out;
    auto dst = ctx.regManager.getRegister(dstVar->getName());

    // Array base
    auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[1]);
    if (!arrVar) return out;
    auto base = ctx.regManager.getRegister(arrVar->getName());

    // Index handling
    if (auto idxConst = std::dynamic_pointer_cast<IRConstantOperand>(ir.operands[2])) {
        int idx = std::atoi(idxConst->toString().c_str());
        int elementSize = 4;
        int offset = idx * elementSize;
        MIPSInstruction lw;
        lw.op = MIPSOp::LW;
        lw.operands = { dst, std::make_shared<Address>(offset, base) };
        out.push_back(std::move(lw));
    } else if (auto idxVar = std::dynamic_pointer_cast<IRVariableOperand>(ir.operands[2])) {
        auto idxReg = ctx.regManager.getRegister(idxVar->getName());
        auto addrReg = ctx.regManager.getVirtualRegister();
        auto calc = calculateArrayAddress(addrReg, base, idxReg, /*elementSize=*/4, ctx);
        out.insert(out.end(), calc.begin(), calc.end());
        MIPSInstruction lw;
        lw.op = MIPSOp::LW;
        lw.operands = { dst, std::make_shared<Address>(0, addrReg) };
        out.push_back(std::move(lw));
    }
    return out;
}

static inline bool isPowerOfTwo(int x){ return x > 0 && (x & (x-1)) == 0; }
static inline int log2i(int x){ int s=0; while(x>1){ x >>= 1; ++s; } return s; }

std::vector<MIPSInstruction> MemorySelector::calculateArrayAddress(std::shared_ptr<Register> resultReg,
                                                                 std::shared_ptr<Register> baseReg,
                                                                 std::shared_ptr<Register> indexReg,
                                                                 int elementSize,
                                                                 SelectionContext& /*ctx*/) {
    // Compute: resultReg = baseReg + indexReg * elementSize
    std::vector<MIPSInstruction> seq;

    if (isPowerOfTwo(elementSize)) {
        int sh = log2i(elementSize);
        MIPSInstruction sll;
        sll.op = MIPSOp::SLL;
        sll.operands = { resultReg, indexReg, std::make_shared<Immediate>(sh) };
        seq.push_back(std::move(sll));
    } else {
        // resultReg = indexReg * elementSize
        MIPSInstruction li;
        li.op = MIPSOp::LI;
        li.operands = { resultReg, std::make_shared<Immediate>(elementSize) };
        seq.push_back(std::move(li));

        MIPSInstruction mul;
        mul.op = MIPSOp::MUL;
        mul.operands = { resultReg, indexReg, resultReg };
        seq.push_back(std::move(mul));
    }

    // resultReg += baseReg
    MIPSInstruction add;
    add.op = MIPSOp::ADD;
    add.operands = { resultReg, resultReg, baseReg };
    seq.push_back(std::move(add));

    return seq;
}

std::vector<MIPSInstruction> MemorySelector::selectImmediateAssign(std::shared_ptr<Register> destReg,
                                                                 int immediateValue,
                                                                 SelectionContext& /*ctx*/) {
    std::vector<MIPSInstruction> out;
    MIPSInstruction li;
    li.op = MIPSOp::LI;
    li.operands = { destReg, std::make_shared<Immediate>(immediateValue) };
    out.push_back(std::move(li));
    return out;
}

std::vector<MIPSInstruction> MemorySelector::selectRegisterAssign(std::shared_ptr<Register> destReg,
                                                                std::shared_ptr<Register> srcReg,
                                                                SelectionContext& /*ctx*/) {
    std::vector<MIPSInstruction> out;
    MIPSInstruction mv;
    mv.op = MIPSOp::MOVE;
    mv.operands = { destReg, srcReg };
    out.push_back(std::move(mv));
    return out;
}

} // namespace ircpp
