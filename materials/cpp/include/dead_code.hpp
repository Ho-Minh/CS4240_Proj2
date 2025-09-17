#ifndef IRCPP_DEAD_CODE_H
#define IRCPP_DEAD_CODE_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir.hpp"
#include "reaching_def.hpp"

bool isCritical(ircpp::IRInstruction::OpCode opCode, ircpp::IRInstruction* inst);

bool isDefiningInstruction(ircpp::IRInstruction::OpCode opCode);

struct DeadCodeAnalysis {
    // Instructions that can be eliminated
    std::unordered_set<int> deadInstructions;
};

struct DeadCodeResult {
    std::vector<std::unordered_map<std::string, DeadCodeAnalysis>> functionResults;
};

DeadCodeResult analyzeDeadCode(const std::vector<ircpp::ControlFlowGraph> &functionCfgs);

#endif //IRCPP_DEAD_CODE_H
