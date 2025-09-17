#include "ir.hpp"
#include "reaching_def.hpp"

#include <optional>

std::optional<std::string> getDefVar(const ircpp::IRInstruction &instr) {
    switch(instr.opCode) {
        case ircpp::IRInstruction::OpCode::ASSIGN:
        case ircpp::IRInstruction::OpCode::ADD:
        case ircpp::IRInstruction::OpCode::SUB:
        case ircpp::IRInstruction::OpCode::MULT:
        case ircpp::IRInstruction::OpCode::DIV:
        case ircpp::IRInstruction::OpCode::AND:
        case ircpp::IRInstruction::OpCode::OR:
        case ircpp::IRInstruction::OpCode::ARRAY_LOAD:
            return dynamic_cast<ircpp::IRVariableOperand *>(instr.operands[0].get())->getName();
        default:
            return std::nullopt;
    }
}

struct GenKill {
    std::unordered_map<std::string, std::unordered_set<int>> gen;
    std::unordered_map<std::string, std::unordered_set<int>> kill;
};

GenKill computeGenKillBlock(const ircpp::ControlFlowGraph &cfg) {
    GenKill res;
    std::unordered_map<std::string, std::unordered_set<int>> defsByVar;
    for(auto &[block_name, block] : cfg.blocks) {
        for(auto &instr : block->instructions) {
            if(auto defVar = getDefVar(*instr)) {
                defsByVar[*defVar].insert(instr->irLineNumber);
            }
        }
    }
    for(auto &[block_name, block] : cfg.blocks) {
        for(auto &instr : block->instructions) {
            if(auto defVar = getDefVar(*instr)) {
                res.gen[block_name].insert(instr->irLineNumber);
                // Kill all other definitions of the same variable
                for(auto line : defsByVar[*defVar]) {
                    if(line != instr->irLineNumber) {
                        res.kill[block_name].insert(line);
                    }
                }
            }
        }
    }
    return res;
}

std::unordered_map<std::string, BasicBlockReachingDef> computeReachingDefs(const ircpp::ControlFlowGraph &cfg) {
    std::unordered_map<std::string, BasicBlockReachingDef> res;
    GenKill genKill = computeGenKillBlock(cfg);
    for(auto &[block_name, _] : cfg.blocks) {
        res[block_name].in = {};
        res[block_name].out = genKill.gen[block_name];
    }
    bool changed;
    do {
        changed = false;
        std::unordered_map<std::string, BasicBlockReachingDef> next_res;
        for(auto &[block_name, block] : cfg.blocks) {
            next_res[block_name].in = {};
            for(auto pred : block->predecessors) {
                for(auto out_elem : res[pred].out) {
                    next_res[block_name].in.insert(out_elem);
                }
            }
            next_res[block_name].out = next_res[block_name].in;
            for(auto kill_elem : genKill.kill[block_name]) {
                next_res[block_name].out.erase(kill_elem);
            }
            for(auto gen_elem : genKill.gen[block_name]) {
                next_res[block_name].out.insert(gen_elem);
            }
            if(next_res[block_name].out != res[block_name].out) {
                changed = true;
            }
        }
        res = next_res;  // CRITICAL: Update res with next_res
    } while(changed);
    return res;
}

ReachingDefOut computeReachingDefs(const std::vector<ircpp::ControlFlowGraph> &functionCfgs) {
    ReachingDefOut res;
    for(auto &cfg : functionCfgs) {
        res.push_back(computeReachingDefs(cfg));
    }
    return res;
}