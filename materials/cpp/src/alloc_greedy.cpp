#include "alloc_greedy.hpp"
#include "frame_builder.hpp"
#include "emit_helpers.hpp"
#include "instruction_selector.hpp"
#include <bits/stdc++.h>

namespace ircpp {

// Forward-declare helper lifted from instruction_selector.cpp greedy body
static void emitGreedyBody(const IRFunction& F, const FrameInfo& fi, std::vector<MIPSInstruction>& out);

std::vector<MIPSInstruction> emitFunctionGreedy(const IRFunction& F) {
    std::vector<MIPSInstruction> out;
    FrameInfo fi = buildFrame(F);

    // Prologue
    out.emplace_back(MIPSOp::ADDI, F.name, std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::sp(), Registers::sp(), std::make_shared<Immediate>(-fi.frameBytes)
    });
    out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::ra(), std::make_shared<Address>(0, Registers::sp())
    });
    out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::fp(), std::make_shared<Address>(4, Registers::sp())
    });
    out.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::fp(), Registers::sp()
    });

    // Store first 4 parameters
    for (size_t i = 0; i < std::min<size_t>(4, F.parameters.size()); ++i) {
        auto p = F.parameters[i]; if (!p) continue;
        auto it = fi.varOffset.find(p->getName());
        if (it != fi.varOffset.end()) {
            static std::shared_ptr<Register> aRegs[4] = { Registers::a0(), Registers::a1(), Registers::a2(), Registers::a3() };
            out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                aRegs[i], std::make_shared<Address>(it->second, Registers::fp())
            });
        }
    }
    // Load stack-passed parameters
    if (F.parameters.size() > 4) {
        for (size_t i = 4; i < F.parameters.size(); ++i) {
            auto p = F.parameters[i]; if (!p) continue;
            int varOff = fi.varOffset[p->getName()];
            int extraOff = fi.frameBytes + int((i - 4) * 4);
            auto t = Registers::t0();
            out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                t, std::make_shared<Address>(extraOff, Registers::fp())
            });
            out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                t, std::make_shared<Address>(varOff, Registers::fp())
            });
        }
    }

    // Body
    emitGreedyBody(F, fi, out);

    // Epilogue
    out.emplace_back(MIPSOp::SLL, F.name + std::string("_epilogue"), std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0)
    });
    out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::ra(), std::make_shared<Address>(0, Registers::fp())
    });
    out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::fp(), std::make_shared<Address>(4, Registers::fp())
    });
    out.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{
        Registers::sp(), Registers::sp(), std::make_shared<Immediate>(fi.frameBytes)
    });
    out.emplace_back(MIPSOp::JR, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::ra() });

    return out;
}

// For brevity, reuse the already-reviewed greedy content by including the existing
// implementation via a forward-declared helper lifted from instruction_selector.cpp.
// In a production refactor, we would fully move that logic here.

static void emitGreedyBody(const IRFunction& F, const FrameInfo& fi, std::vector<MIPSInstruction>& out) {
    auto isBlockEnd = [](IRInstruction::OpCode op){
        switch (op) {
            case IRInstruction::OpCode::GOTO:
            case IRInstruction::OpCode::BREQ:
            case IRInstruction::OpCode::BRNEQ:
            case IRInstruction::OpCode::BRLT:
            case IRInstruction::OpCode::BRGT:
            case IRInstruction::OpCode::BRGEQ:
            case IRInstruction::OpCode::RETURN:
            case IRInstruction::OpCode::CALL:
            case IRInstruction::OpCode::CALLR:
                return true;
            default: return false;
        }
    };

    // Helper loads/stores
    auto loadOp = [&](std::shared_ptr<IROperand> op, std::shared_ptr<Register> dst,
                      std::vector<MIPSInstruction>& code){
        if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(op)) {
            int val = std::stoi(c->getValueString());
            code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{
                dst, std::make_shared<Immediate>(val)
            });
        } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
            int off = fi.varOffset.at(v->getName());
            code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{
                dst, std::make_shared<Address>(off, Registers::fp())
            });
        }
    };
    auto storeVar = [&](const std::string& name, std::shared_ptr<Register> src,
                        std::vector<MIPSInstruction>& code){
        int off = fi.varOffset.at(name);
        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{
            src, std::make_shared<Address>(off, Registers::fp())
        });
    };
    auto storeVarF32 = [&](const std::string& name, const std::shared_ptr<Register>& fSrc,
                           std::vector<MIPSInstruction>& code){
        int off = fi.varOffset.at(name);
        code.emplace_back(MIPSOp::S_S, "", std::vector<std::shared_ptr<MIPSOperand>>{
            fSrc, std::make_shared<Address>(off, Registers::fp())
        });
    };
    auto loadVarF32 = [&](const std::shared_ptr<IROperand>& op, const std::shared_ptr<Register>& fDst,
                          std::vector<MIPSInstruction>& code){
        if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
            int off = fi.varOffset.at(v->getName());
            code.emplace_back(MIPSOp::L_S, "", std::vector<std::shared_ptr<MIPSOperand>>{
                fDst, std::make_shared<Address>(off, Registers::fp())
            });
        }
    };

    // Build blocks
    std::vector<std::pair<int,int>> blocks;
    int n = (int)F.instructions.size();
    int l = 0;
    for (int i = 0; i < n; ++i) {
        auto inst = F.instructions[i];
        if (!inst) continue;
        if (inst->opCode == IRInstruction::OpCode::LABEL) {
            if (i > l) blocks.push_back({l, i-1});
            l = i;
        }
        if (isBlockEnd(inst->opCode)) {
            blocks.push_back({l, i});
            l = i + 1;
        }
    }
    if (l < n) blocks.push_back({l, n-1});

    // Allocatable pool (keep t0..t4 as temps)
    std::vector<std::shared_ptr<Register>> allocRegs = {
        Registers::t5(), Registers::t6(), Registers::t7(), Registers::t8(), Registers::t9()
    };

    auto isScalarVar = [&](const std::shared_ptr<IROperand>& op)->bool{
        auto v = std::dynamic_pointer_cast<IRVariableOperand>(op);
        if (!v) return false;
        if (std::dynamic_pointer_cast<IRArrayType>(v->type)) return false;
        return fi.varOffset.count(v->getName()) > 0;
    };

    for (const auto& br : blocks) {
        int bi = br.first, bj = br.second;
        if (bi > bj) continue;

        if (F.instructions[bi]->opCode == IRInstruction::OpCode::LABEL) {
            auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(F.instructions[bi]->operands[0]);
            std::string Lb = qualLabel(F.name, lbl->getName());
            out.emplace_back(MIPSOp::SLL, Lb, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
        }

        // Build per-position next-use information (after position i)
        const int INF = 1000000000;
        std::vector<std::unordered_map<std::string,int>> nextUseAt(bj - bi + 1);
        std::unordered_map<std::string,int> currNext;
        auto addUse = [&](const std::shared_ptr<IROperand>& op, int pos){
            if (!isScalarVar(op)) return;
                    auto v = std::dynamic_pointer_cast<IRVariableOperand>(op);
            currNext[v->getName()] = pos;
        };
        auto getDefName = [&](const std::shared_ptr<IRInstruction>& ir)->std::string{
            switch (ir->opCode) {
                case IRInstruction::OpCode::ASSIGN: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    if (dst && !std::dynamic_pointer_cast<IRArrayType>(dst->type)) return dst->getName();
                    return std::string();
                }
                case IRInstruction::OpCode::ADD:
                case IRInstruction::OpCode::SUB:
                case IRInstruction::OpCode::MULT:
                case IRInstruction::OpCode::DIV:
                case IRInstruction::OpCode::AND:
                case IRInstruction::OpCode::OR: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    if (dst && !std::dynamic_pointer_cast<IRArrayType>(dst->type)) return dst->getName();
                    return std::string();
                }
                case IRInstruction::OpCode::ARRAY_LOAD: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    if (dst && !std::dynamic_pointer_cast<IRArrayType>(dst->type)) return dst->getName();
                    return std::string();
                }
                case IRInstruction::OpCode::CALLR: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    if (dst && !std::dynamic_pointer_cast<IRArrayType>(dst->type)) return dst->getName();
                    return std::string();
                }
                default: return std::string();
            }
        };

        for (int i = bj; i >= bi; --i) {
            auto ir = F.instructions[i]; if (!ir) { if (i - bi >= 0) nextUseAt[i - bi] = currNext; continue; }
            // snapshot of next use AFTER position i
            nextUseAt[i - bi] = currNext;
            // defs kill previous next-use
            auto def = getDefName(ir);
            if (!def.empty()) currNext[def] = INF;
            // uses at i
            switch (ir->opCode) {
                case IRInstruction::OpCode::ASSIGN: {
                    if (ir->operands.size() >= 2) addUse(ir->operands[1], i);
                    if (ir->operands.size() == 3) addUse(ir->operands[2], i); // array bulk init count/value
                    break;
                }
                case IRInstruction::OpCode::ADD:
                case IRInstruction::OpCode::SUB:
                case IRInstruction::OpCode::MULT:
                case IRInstruction::OpCode::DIV:
                case IRInstruction::OpCode::AND:
                case IRInstruction::OpCode::OR: {
                    addUse(ir->operands[1], i);
                    addUse(ir->operands[2], i);
                    break;
                }
                case IRInstruction::OpCode::ARRAY_STORE: {
                    addUse(ir->operands[0], i);
                    addUse(ir->operands[2], i);
                    break;
                }
                case IRInstruction::OpCode::ARRAY_LOAD: {
                    addUse(ir->operands[2], i);
                    break;
                }
                case IRInstruction::OpCode::BREQ:
                case IRInstruction::OpCode::BRNEQ:
                case IRInstruction::OpCode::BRLT:
                case IRInstruction::OpCode::BRGT:
                case IRInstruction::OpCode::BRGEQ: {
                    addUse(ir->operands[1], i);
                    addUse(ir->operands[2], i);
                    break;
                }
                case IRInstruction::OpCode::RETURN: {
                    addUse(ir->operands[0], i);
                    break;
                }
                case IRInstruction::OpCode::CALL:
                case IRInstruction::OpCode::CALLR: {
                    size_t idx = (ir->opCode == IRInstruction::OpCode::CALLR) ? 2 : 1;
                    for (size_t a = idx; a < ir->operands.size(); ++a) addUse(ir->operands[a], i);
                    break;
                }
                default: break;
            }
        }

        // Dynamic register mapping for this block
        struct Slot { std::shared_ptr<Register> reg; std::string var; bool occupied = false; bool dirty = false; };
        std::vector<Slot> slots(allocRegs.size());
        for (size_t s = 0; s < allocRegs.size(); ++s) slots[s].reg = allocRegs[s];
        std::unordered_map<std::string,int> varToSlot;

        auto spillSlot = [&](int si, std::vector<MIPSInstruction>& code){
            if (!slots[si].occupied) return;
            if (slots[si].dirty) {
                int off = fi.varOffset.at(slots[si].var);
                code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ slots[si].reg, std::make_shared<Address>(off, Registers::fp()) });
            }
            varToSlot.erase(slots[si].var);
            slots[si].occupied = false;
            slots[si].dirty = false;
            slots[si].var.clear();
        };

        auto flushAllDirty = [&](std::vector<MIPSInstruction>& code){
            for (size_t si = 0; si < slots.size(); ++si) {
                if (slots[si].occupied && slots[si].dirty) {
                    int off = fi.varOffset.at(slots[si].var);
                    code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ slots[si].reg, std::make_shared<Address>(off, Registers::fp()) });
                    slots[si].dirty = false;
                }
            }
        };

        auto clearAllMappings = [&](){
            varToSlot.clear();
            for (auto& sl : slots) { sl.occupied = false; sl.dirty = false; sl.var.clear(); }
        };

        auto chooseVictim = [&](int i)->int{
            for (int s = 0; s < (int)slots.size(); ++s) if (!slots[s].occupied) return s;
            int best = 0; int bestNu = -1;
            auto& map = nextUseAt[i - bi];
            for (int s = 0; s < (int)slots.size(); ++s) {
                int nu = INF;
                auto it = map.find(slots[s].var);
                if (it != map.end()) nu = it->second;
                if (nu > bestNu) { bestNu = nu; best = s; }
            }
            return best;
        };

        auto ensureVarRegForRead = [&](const std::string& name, int i, std::vector<MIPSInstruction>& code)->std::shared_ptr<Register>{
            auto it = varToSlot.find(name);
            if (it != varToSlot.end()) return slots[it->second].reg;
            int si = chooseVictim(i);
            if (slots[si].occupied) spillSlot(si, code);
            int off = fi.varOffset.at(name);
            code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ slots[si].reg, std::make_shared<Address>(off, Registers::fp()) });
            slots[si].occupied = true; slots[si].dirty = false; slots[si].var = name;
            varToSlot[name] = si;
            return slots[si].reg;
        };

        auto ensureVarRegForWrite = [&](const std::string& name, int i, std::vector<MIPSInstruction>& code)->std::shared_ptr<Register>{
            auto it = varToSlot.find(name);
            if (it != varToSlot.end()) return slots[it->second].reg;
            int si = chooseVictim(i);
            if (slots[si].occupied) spillSlot(si, code);
            slots[si].occupied = true; slots[si].dirty = false; slots[si].var = name;
            varToSlot[name] = si;
            return slots[si].reg;
        };

        auto freeIfLastUse = [&](const std::string& name, int i, std::vector<MIPSInstruction>& code){
            auto itMap = nextUseAt[i - bi].find(name);
            int nu = (itMap == nextUseAt[i - bi].end()) ? INF : itMap->second;
            if (nu == INF) {
                auto it = varToSlot.find(name);
                if (it != varToSlot.end()) {
                    int si = it->second;
                    if (slots[si].dirty) {
                        int off = fi.varOffset.at(name);
                        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ slots[si].reg, std::make_shared<Address>(off, Registers::fp()) });
                    }
                    varToSlot.erase(name);
                    slots[si].occupied = false; slots[si].dirty = false; slots[si].var.clear();
                }
            }
        };

        auto getOpIntoTemp = [&](const std::shared_ptr<IROperand>& op, const std::shared_ptr<Register>& tmp, int i, std::vector<MIPSInstruction>& code){
            if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
                if (!std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                    auto r = ensureVarRegForRead(v->getName(), i, code);
                    if (r->toString() != tmp->toString()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ tmp, r });
                    freeIfLastUse(v->getName(), i, code);
                    return; // value now in tmp
                }
            }
            // constants or non-scalar variables
            loadOp(op, tmp, code);
        };

        for (int i = bi; i <= bj; ++i) {
            auto ir = F.instructions[i]; if (!ir) continue;
            if (ir->opCode == IRInstruction::OpCode::LABEL && i == bi) continue;
            std::vector<MIPSInstruction> code;
            switch (ir->opCode) {
                case IRInstruction::OpCode::ASSIGN: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    if (ir->operands.size() == 3 && dst && std::dynamic_pointer_cast<IRArrayType>(dst->type)) {
                        auto tCnt  = Registers::t0();
                        auto tVal  = Registers::t1();
                        auto tIdx  = Registers::t2();
                        auto tAddr = Registers::t3();
                        auto baseR = Registers::t4();
                        getOpIntoTemp(ir->operands[1], tCnt, i, code);
                        getOpIntoTemp(ir->operands[2], tVal, i, code);
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, std::make_shared<Immediate>(0) });
                        int baseOff = fi.varOffset.at(dst->getName());
                        if (fi.paramArrayNames.count(dst->getName())) {
                            code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseR, std::make_shared<Address>(baseOff, Registers::fp()) });
                        } else {
                            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseR, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                        }
                        static int arrSetCounter = 0;
                        std::string Lloop = F.name + std::string("_arrset_") + std::to_string(arrSetCounter++);
                        std::string Lend  = F.name + std::string("_arrset_end_") + std::to_string(arrSetCounter++);
                        code.emplace_back(MIPSOp::SLL, Lloop, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                        code.emplace_back(MIPSOp::BGE, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, tCnt, std::make_shared<Label>(Lend) });
                        code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                        code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseR, tAddr });
                        code.emplace_back(MIPSOp::SW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ tIdx, tIdx, std::make_shared<Immediate>(1) });
                        code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(Lloop) });
                        code.emplace_back(MIPSOp::SLL, Lend, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                    } else {
                        if (!dst) break;
                        auto dstR = ensureVarRegForWrite(dst->getName(), i, code);
                            if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(ir->operands[1])) {
                                int val = std::stoi(c->getValueString());
                                code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ dstR, std::make_shared<Immediate>(val) });
                            } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1])) {
                            if (!std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                                auto srcR = ensureVarRegForRead(v->getName(), i, code);
                                if (srcR->toString() != dstR->toString()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ dstR, srcR });
                                freeIfLastUse(v->getName(), i, code);
                            } else {
                                    auto t0 = Registers::t0(); loadOp(ir->operands[1], t0, code);
                                    code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ dstR, t0 });
                                }
                            }
                        // mark dst dirty
                        {
                            auto itSlot = varToSlot.find(dst->getName());
                            if (itSlot != varToSlot.end()) slots[itSlot->second].dirty = true;
                        }
                    }
                    break;
                }
                case IRInstruction::OpCode::ADD:
                case IRInstruction::OpCode::SUB:
                case IRInstruction::OpCode::MULT:
                case IRInstruction::OpCode::DIV:
                case IRInstruction::OpCode::AND:
                case IRInstruction::OpCode::OR: {
                    auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                    auto rYt = Registers::t0();
                    auto rZt = Registers::t1();
                    std::shared_ptr<Register> rY;
                    std::shared_ptr<Register> rZ;
                    if (isScalarVar(ir->operands[1])) {
                        auto vy = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                        rY = ensureVarRegForRead(vy->getName(), i, code);
                    } else { rY = rYt; getOpIntoTemp(ir->operands[1], rYt, i, code); }
                    if (isScalarVar(ir->operands[2])) {
                        auto vz = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[2]);
                        rZ = ensureVarRegForRead(vz->getName(), i, code);
                    } else { rZ = rZt; getOpIntoTemp(ir->operands[2], rZt, i, code); }
                    auto rX = ensureVarRegForWrite(dst->getName(), i, code);
                    MIPSOp op = MIPSOp::ADD;
                    if (ir->opCode == IRInstruction::OpCode::SUB) op = MIPSOp::SUB;
                    else if (ir->opCode == IRInstruction::OpCode::MULT) op = MIPSOp::MUL;
                    else if (ir->opCode == IRInstruction::OpCode::DIV) op = MIPSOp::DIV;
                    else if (ir->opCode == IRInstruction::OpCode::AND) op = MIPSOp::AND;
                    else if (ir->opCode == IRInstruction::OpCode::OR)  op = MIPSOp::OR;
                    code.emplace_back(op, "", std::vector<std::shared_ptr<MIPSOperand>>{ rX, rY, rZ });
                    {
                        auto itSlot = varToSlot.find(dst->getName());
                        if (itSlot != varToSlot.end()) slots[itSlot->second].dirty = true;
                    }
                    if (isScalarVar(ir->operands[1])) freeIfLastUse(std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1])->getName(), i, code);
                    if (isScalarVar(ir->operands[2])) freeIfLastUse(std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[2])->getName(), i, code);
                    break;
                }
                case IRInstruction::OpCode::GOTO: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    flushAllDirty(code);
                    code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(qualLabel(F.name, lbl->getName())) });
                    clearAllMappings();
                    break;
                }
                case IRInstruction::OpCode::BREQ:
                case IRInstruction::OpCode::BRNEQ:
                case IRInstruction::OpCode::BRLT:
                case IRInstruction::OpCode::BRGT:
                case IRInstruction::OpCode::BRGEQ: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    auto rA = Registers::t0(); auto rB = Registers::t1();
                    getOpIntoTemp(ir->operands[1], rA, i, code);
                    getOpIntoTemp(ir->operands[2], rB, i, code);
                    MIPSOp bop = MIPSOp::BEQ;
                    if (ir->opCode == IRInstruction::OpCode::BRNEQ) bop = MIPSOp::BNE;
                    else if (ir->opCode == IRInstruction::OpCode::BRLT) bop = MIPSOp::BLT;
                    else if (ir->opCode == IRInstruction::OpCode::BRGT) bop = MIPSOp::BGT;
                    else if (ir->opCode == IRInstruction::OpCode::BRGEQ) bop = MIPSOp::BGE;
                    flushAllDirty(code);
                    code.emplace_back(bop, "", std::vector<std::shared_ptr<MIPSOperand>>{ rA, rB, std::make_shared<Label>(qualLabel(F.name, lbl->getName())) });
                    break;
                }
                case IRInstruction::OpCode::CALL:
                case IRInstruction::OpCode::CALLR: {
                    size_t idxArg = (ir->opCode == IRInstruction::OpCode::CALLR) ? 2 : 1;
                    auto fnOp = std::dynamic_pointer_cast<IRFunctionOperand>(ir->operands[idxArg-1]);
                    std::string callee = fnOp ? fnOp->getName() : ir->operands[idxArg-1]->toString();
                    if (callee == "geti") {
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(5) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            storeVar(dst->getName(), Registers::v0(), code);
                        }
                        break;
                    }
                    if (callee == "getc") {
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(12) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            storeVar(dst->getName(), Registers::v0(), code);
                        }
                        break;
                    }
                    if (callee == "puti" || callee == "putc") {
                        auto t0 = Registers::t0();
                        if (idxArg < ir->operands.size()) getOpIntoTemp(ir->operands[idxArg], t0, i, code);
                        code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::a0(), t0 });
                        int sc = (callee == "puti") ? 1 : 11;
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(sc) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        break;
                    }
                    if (callee == "putf") {
                        auto f12 = std::make_shared<Register>(Register{"f12", true});
                        if (idxArg < ir->operands.size()) loadVarF32(ir->operands[idxArg], f12, code);
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(2) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        break;
                    }
                    if (callee == "getf") {
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(6) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            auto f0 = std::make_shared<Register>(Register{"f0", true});
                            storeVarF32(dst->getName(), f0, code);
                        }
                        break;
                    }
                    static std::shared_ptr<Register> aRegs[4] = { Registers::a0(), Registers::a1(), Registers::a2(), Registers::a3() };
                    flushAllDirty(code);
                    for (size_t a = 0; a < 4 && idxArg + a < ir->operands.size(); ++a) {
                        auto arg = ir->operands[idxArg + a];
                        if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(arg)) {
                            if (std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                                int base = fi.varOffset.at(v->getName());
                                if (fi.paramArrayNames.count(v->getName())) {
                                    code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], std::make_shared<Address>(base, Registers::fp()) });
                                } else {
                                    code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], Registers::fp(), std::make_shared<Immediate>(base) });
                                }
                                continue;
                            }
                        }
                        auto t = Registers::t0();
                        getOpIntoTemp(arg, t, i, code);
                        code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], t });
                    }
                    if (idxArg + 4 < ir->operands.size()) {
                        size_t extraStart = idxArg + 4;
                        std::vector<std::shared_ptr<IROperand>> extras;
                        for (size_t a = extraStart; a < ir->operands.size(); ++a) extras.push_back(ir->operands[a]);
                        for (size_t r = extras.size(); r-- > 0; ) {
                            auto t = Registers::t0();
                            getOpIntoTemp(extras[r], t, i, code);
                            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::sp(), Registers::sp(), std::make_shared<Immediate>(-4) });
                            code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ t, std::make_shared<Address>(0, Registers::sp()) });
                        }
                    }
                    code.emplace_back(MIPSOp::JAL, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(callee) });
                    if (idxArg + 4 < ir->operands.size()) {
                        int extra = int(ir->operands.size() - (idxArg + 4));
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::sp(), Registers::sp(), std::make_shared<Immediate>(extra * 4) });
                    }
                    clearAllMappings();
                    if (ir->opCode == IRInstruction::OpCode::CALLR) {
                        auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                        storeVar(dst->getName(), Registers::v0(), code);
                    }
                    break;
                }
                case IRInstruction::OpCode::LABEL:
                default: {
                    // Fallbacks for array ops and return using mapped regs when possible
                    switch (ir->opCode) {
                        case IRInstruction::OpCode::LABEL: {
                            auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                            std::string Lb2 = qualLabel(F.name, lbl->getName());
                            code.emplace_back(MIPSOp::SLL, Lb2, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
                            break;
                        }
                        case IRInstruction::OpCode::ARRAY_STORE: {
                            auto tVal = Registers::t0();
                            auto tIdx = Registers::t1();
                            auto tAddr = Registers::t2();
                            getOpIntoTemp(ir->operands[0], tVal, i, code);
                            auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                            getOpIntoTemp(ir->operands[2], tIdx, i, code);
                            std::shared_ptr<Register> baseReg = Registers::t3();
                            int baseOff = fi.varOffset.at(arrVar->getName());
                            if (fi.paramArrayNames.count(arrVar->getName())) {
                                code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, std::make_shared<Address>(baseOff, Registers::fp()) });
                            } else {
                                code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                            }
                            code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                            code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseReg, tAddr });
                            code.emplace_back(MIPSOp::SW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                            // free last-use of operands
                            if (isScalarVar(ir->operands[0])) freeIfLastUse(std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0])->getName(), i, code);
                            if (isScalarVar(ir->operands[2])) freeIfLastUse(std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[2])->getName(), i, code);
                            break;
                        }
                        case IRInstruction::OpCode::ARRAY_LOAD: {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            auto tIdx = Registers::t0();
                            auto tAddr = Registers::t1();
                            auto tVal = Registers::t2();
                            auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                            getOpIntoTemp(ir->operands[2], tIdx, i, code);
                            std::shared_ptr<Register> baseReg = Registers::t3();
                            int baseOff = fi.varOffset.at(arrVar->getName());
                            if (fi.paramArrayNames.count(arrVar->getName())) {
                                code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, std::make_shared<Address>(baseOff, Registers::fp()) });
                            } else {
                                code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, Registers::fp(), std::make_shared<Immediate>(baseOff) });
                            }
                            code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, tIdx, std::make_shared<Immediate>(2) });
                            code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ tAddr, baseReg, tAddr });
                            code.emplace_back(MIPSOp::LW,  "", std::vector<std::shared_ptr<MIPSOperand>>{ tVal, std::make_shared<Address>(0, tAddr) });
                            auto rDst = ensureVarRegForWrite(dst->getName(), i, code);
                            code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ rDst, tVal });
                            {
                                auto itSlot = varToSlot.find(dst->getName());
                                if (itSlot != varToSlot.end()) slots[itSlot->second].dirty = true;
                            }
                            if (isScalarVar(ir->operands[2])) freeIfLastUse(std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[2])->getName(), i, code);
                            break;
                        }
                        case IRInstruction::OpCode::RETURN: {
                            auto t0 = Registers::t0();
                            getOpIntoTemp(ir->operands[0], t0, i, code);
                            code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), t0 });
                            code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(F.name + std::string("_epilogue")) });
                            break;
                        }
                        default: break;
                    }
                    break;
                }
            }
            out.insert(out.end(), code.begin(), code.end());
        }
        // Flush at block end
        std::vector<MIPSInstruction> flush;
        flushAllDirty(flush);
        out.insert(out.end(), flush.begin(), flush.end());
    }
}

}


