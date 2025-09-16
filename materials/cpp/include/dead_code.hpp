#ifndef IRCPP_DEAD_CODE_H
#define IRCPP_DEAD_CODE_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir.hpp"
#include "reaching_def.hpp"

struct DeadCodeAnalysis {
    // Instructions that are unreachable (dead code)
    std::unordered_set<int> unreachableInstructions;
    
    // Instructions whose results are never used (dead assignments)
    std::unordered_set<int> unusedAssignments;
    
    // Instructions that can be eliminated
    std::unordered_set<int> deadInstructions;
};

struct DeadCodeResult {
    std::vector<std::unordered_map<std::string, DeadCodeAnalysis>> functionResults;
    
    // Summary statistics
    int totalDeadInstructions = 0;
    int totalUnreachableInstructions = 0;
    int totalUnusedAssignments = 0;
};

// Main dead code elimination functions
DeadCodeResult analyzeDeadCode(const std::vector<ircpp::ControlFlowGraph> &functionCfgs);
DeadCodeResult eliminateDeadCode(const std::vector<ircpp::ControlFlowGraph> &functionCfgs);

// Helper functions for different types of dead code
DeadCodeAnalysis findUnreachableInstructions(const ircpp::ControlFlowGraph &cfg);
DeadCodeAnalysis findUnusedAssignments(const ircpp::ControlFlowGraph &cfg, const std::unordered_map<std::string, BasicBlockReachingDef> &reachingDefs);

// Utility functions
bool isInstructionReachable(int lineNumber, const ircpp::ControlFlowGraph &cfg, const std::unordered_map<std::string, BasicBlockReachingDef> &reachingDefs);
bool isAssignmentUsed(int lineNumber, const ircpp::ControlFlowGraph &cfg, const std::unordered_map<std::string, BasicBlockReachingDef> &reachingDefs);
std::unordered_set<int> getAllReachableInstructions(const ircpp::ControlFlowGraph &cfg);

#endif //IRCPP_DEAD_CODE_H
