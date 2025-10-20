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

    auto emitOpWithRegs = [&](const std::shared_ptr<IROperand>& op,
                              const std::unordered_map<std::string,std::shared_ptr<Register>>& map,
                              const std::shared_ptr<Register>& tmp,
                              std::vector<MIPSInstruction>& code){
        if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
            auto it = map.find(v->getName());
            if (it != map.end()) {
                if (tmp) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ tmp, it->second });
                return;
            }
        }
        if (tmp) loadOp(op, tmp, code);
    };

    for (const auto& br : blocks) {
        int bi = br.first, bj = br.second;
        if (bi > bj) continue;

        if (F.instructions[bi]->opCode == IRInstruction::OpCode::LABEL) {
            auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(F.instructions[bi]->operands[0]);
            std::string Lb = qualLabel(F.name, lbl->getName());
            out.emplace_back(MIPSOp::SLL, Lb, std::vector<std::shared_ptr<MIPSOperand>>{ Registers::zero(), Registers::zero(), std::make_shared<Immediate>(0) });
        }

        std::unordered_map<std::string,int> useCnt;
        for (int i = bi; i <= bj; ++i) {
            auto inst = F.instructions[i]; if (!inst) continue;
            for (auto& op : inst->operands) {
                if (isScalarVar(op)) {
                    auto v = std::dynamic_pointer_cast<IRVariableOperand>(op);
                    useCnt[v->getName()]++;
                }
            }
        }
        std::vector<std::pair<std::string,int>> pairs(useCnt.begin(), useCnt.end());
        std::sort(pairs.begin(), pairs.end(), [](auto& a, auto& b){ return a.second > b.second; });
        size_t K = std::min(pairs.size(), allocRegs.size());
        std::unordered_map<std::string,std::shared_ptr<Register>> regMap;
        for (size_t i = 0; i < K; ++i) regMap[pairs[i].first] = allocRegs[i];

        // Load mapped vars at block entry
        for (auto& kv : regMap) {
            int off = fi.varOffset.at(kv.first);
            out.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ kv.second, std::make_shared<Address>(off, Registers::fp()) });
        }

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
                        emitOpWithRegs(ir->operands[1], regMap, tCnt, code);
                        emitOpWithRegs(ir->operands[2], regMap, tVal, code);
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
                        auto it = regMap.find(dst ? dst->getName() : std::string());
                        if (it != regMap.end()) {
                            auto dstR = it->second;
                            if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(ir->operands[1])) {
                                int val = std::stoi(c->getValueString());
                                code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ dstR, std::make_shared<Immediate>(val) });
                            } else if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1])) {
                                auto it2 = regMap.find(v->getName());
                                if (it2 != regMap.end()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ dstR, it2->second });
                                else {
                                    auto t0 = Registers::t0(); loadOp(ir->operands[1], t0, code);
                                    code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ dstR, t0 });
                                }
                            }
                        } else {
                            auto t0 = Registers::t0();
                            loadOp(ir->operands[1], t0, code);
                            storeVar(dst->getName(), t0, code);
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
                    auto itDst = regMap.find(dst ? dst->getName() : std::string());
                    auto rY = Registers::t0();
                    auto rZ = Registers::t1();
                    if (auto vy = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1])) {
                        auto it = regMap.find(vy->getName());
                        if (it != regMap.end()) rY = it->second; else emitOpWithRegs(ir->operands[1], regMap, rY, code);
                    } else emitOpWithRegs(ir->operands[1], regMap, rY, code);
                    if (auto vz = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[2])) {
                        auto it = regMap.find(vz->getName());
                        if (it != regMap.end()) rZ = it->second; else emitOpWithRegs(ir->operands[2], regMap, rZ, code);
                    } else emitOpWithRegs(ir->operands[2], regMap, rZ, code);
                    auto rX = itDst != regMap.end() ? itDst->second : Registers::t2();
                    MIPSOp op = MIPSOp::ADD;
                    if (ir->opCode == IRInstruction::OpCode::SUB) op = MIPSOp::SUB;
                    else if (ir->opCode == IRInstruction::OpCode::MULT) op = MIPSOp::MUL;
                    else if (ir->opCode == IRInstruction::OpCode::DIV) op = MIPSOp::DIV;
                    else if (ir->opCode == IRInstruction::OpCode::AND) op = MIPSOp::AND;
                    else if (ir->opCode == IRInstruction::OpCode::OR)  op = MIPSOp::OR;
                    code.emplace_back(op, "", std::vector<std::shared_ptr<MIPSOperand>>{ rX, rY, rZ });
                    if (itDst == regMap.end()) storeVar(dst->getName(), rX, code);
                    break;
                }
                case IRInstruction::OpCode::GOTO: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    for (auto& kv : regMap) {
                        int off = fi.varOffset.at(kv.first);
                        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ kv.second, std::make_shared<Address>(off, Registers::fp()) });
                    }
                    code.emplace_back(MIPSOp::J, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(qualLabel(F.name, lbl->getName())) });
                    break;
                }
                case IRInstruction::OpCode::BREQ:
                case IRInstruction::OpCode::BRNEQ:
                case IRInstruction::OpCode::BRLT:
                case IRInstruction::OpCode::BRGT:
                case IRInstruction::OpCode::BRGEQ: {
                    auto lbl = std::dynamic_pointer_cast<IRLabelOperand>(ir->operands[0]);
                    auto rA = Registers::t0(); auto rB = Registers::t1();
                    emitOpWithRegs(ir->operands[1], regMap, rA, code);
                    emitOpWithRegs(ir->operands[2], regMap, rB, code);
                    MIPSOp bop = MIPSOp::BEQ;
                    if (ir->opCode == IRInstruction::OpCode::BRNEQ) bop = MIPSOp::BNE;
                    else if (ir->opCode == IRInstruction::OpCode::BRLT) bop = MIPSOp::BLT;
                    else if (ir->opCode == IRInstruction::OpCode::BRGT) bop = MIPSOp::BGT;
                    else if (ir->opCode == IRInstruction::OpCode::BRGEQ) bop = MIPSOp::BGE;
                    for (auto& kv : regMap) {
                        int off = fi.varOffset.at(kv.first);
                        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ kv.second, std::make_shared<Address>(off, Registers::fp()) });
                    }
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
                            auto it = regMap.find(dst ? dst->getName() : std::string());
                            if (it != regMap.end()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ it->second, Registers::v0() });
                            else storeVar(dst->getName(), Registers::v0(), code);
                        }
                        break;
                    }
                    if (callee == "getc") {
                        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::v0(), std::make_shared<Immediate>(12) });
                        code.emplace_back(MIPSOp::SYSCALL, "", std::vector<std::shared_ptr<MIPSOperand>>{});
                        if (ir->opCode == IRInstruction::OpCode::CALLR) {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            auto it = regMap.find(dst ? dst->getName() : std::string());
                            if (it != regMap.end()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ it->second, Registers::v0() });
                            else storeVar(dst->getName(), Registers::v0(), code);
                        }
                        break;
                    }
                    if (callee == "puti" || callee == "putc") {
                        auto t0 = Registers::t0();
                        if (idxArg < ir->operands.size()) emitOpWithRegs(ir->operands[idxArg], regMap, t0, code);
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
                    for (auto& kv : regMap) {
                        int off = fi.varOffset.at(kv.first);
                        code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ kv.second, std::make_shared<Address>(off, Registers::fp()) });
                    }
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
                        emitOpWithRegs(arg, regMap, t, code);
                        code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ aRegs[a], t });
                    }
                    if (idxArg + 4 < ir->operands.size()) {
                        size_t extraStart = idxArg + 4;
                        std::vector<std::shared_ptr<IROperand>> extras;
                        for (size_t a = extraStart; a < ir->operands.size(); ++a) extras.push_back(ir->operands[a]);
                        for (size_t r = extras.size(); r-- > 0; ) {
                            auto t = Registers::t0();
                            emitOpWithRegs(extras[r], regMap, t, code);
                            code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::sp(), Registers::sp(), std::make_shared<Immediate>(-4) });
                            code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ t, std::make_shared<Address>(0, Registers::sp()) });
                        }
                    }
                    code.emplace_back(MIPSOp::JAL, "", std::vector<std::shared_ptr<MIPSOperand>>{ std::make_shared<Label>(callee) });
                    if (idxArg + 4 < ir->operands.size()) {
                        int extra = int(ir->operands.size() - (idxArg + 4));
                        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ Registers::sp(), Registers::sp(), std::make_shared<Immediate>(extra * 4) });
                    }
                    for (auto& kv : regMap) {
                        int off = fi.varOffset.at(kv.first);
                        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ kv.second, std::make_shared<Address>(off, Registers::fp()) });
                    }
                    if (ir->opCode == IRInstruction::OpCode::CALLR) {
                        auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                        auto it = regMap.find(dst ? dst->getName() : std::string());
                        if (it != regMap.end()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ it->second, Registers::v0() });
                        else storeVar(dst->getName(), Registers::v0(), code);
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
                            emitOpWithRegs(ir->operands[0], regMap, tVal, code);
                            auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                            emitOpWithRegs(ir->operands[2], regMap, tIdx, code);
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
                            break;
                        }
                        case IRInstruction::OpCode::ARRAY_LOAD: {
                            auto dst = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[0]);
                            auto tIdx = Registers::t0();
                            auto tAddr = Registers::t1();
                            auto tVal = Registers::t2();
                            auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(ir->operands[1]);
                            emitOpWithRegs(ir->operands[2], regMap, tIdx, code);
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
                            auto it = regMap.find(dst ? dst->getName() : std::string());
                            if (it != regMap.end()) code.emplace_back(MIPSOp::MOVE, "", std::vector<std::shared_ptr<MIPSOperand>>{ it->second, tVal });
                            else storeVar(dst->getName(), tVal, code);
                            break;
                        }
                        case IRInstruction::OpCode::RETURN: {
                            auto t0 = Registers::t0();
                            emitOpWithRegs(ir->operands[0], regMap, t0, code);
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
        for (auto& kv : regMap) {
            int off = fi.varOffset.at(kv.first);
            out.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ kv.second, std::make_shared<Address>(off, Registers::fp()) });
        }
    }
}

}


