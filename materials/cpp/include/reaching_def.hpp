#ifndef IRCPP_REACHING_DEF_H
#define IRCPP_REACHING_DEF_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir.hpp"

struct BasicBlockReachingDef {
    std::unordered_set<int> in;
    std::unordered_set<int> out;
};

using ReachingDefOut = std::vector<std::unordered_map<std::string, BasicBlockReachingDef>>;

ReachingDefOut computeReachingDefs(const std::vector<ircpp::ControlFlowGraph> &functionCfgs);

#endif //IRCPP_REACHING_DEF_H