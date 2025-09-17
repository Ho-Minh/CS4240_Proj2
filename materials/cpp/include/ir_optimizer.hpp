#ifndef IRCPP_IR_OPTIMIZER_H
#define IRCPP_IR_OPTIMIZER_H
#include <string>
#include <unordered_set>
#include <vector>

#include "ir.hpp"
#include "reaching_def.hpp"
#include "dead_code.hpp"

class IROptimizer {
public:
    // Optimize a program and return a new optimized version
    static std::shared_ptr<ircpp::IRProgram> optimizeProgram(const ircpp::IRProgram& originalProgram);
    
    // Optimize a single function
    static std::shared_ptr<ircpp::IRFunction> optimizeFunction(const ircpp::IRFunction& originalFunction);
    
    // Write optimized program to file
    static void writeOptimizedProgram(const ircpp::IRProgram& optimizedProgram, const std::string& filename);
    
    // Write optimized program using new format (fallback)
    static void writeOptimizedProgramNewFormat(const ircpp::IRProgram& optimizedProgram, const std::string& filename);
    

private:
    // Helper functions
    static std::vector<std::shared_ptr<ircpp::IRInstruction>> eliminateDeadInstructions(
        const std::vector<std::shared_ptr<ircpp::IRInstruction>>& instructions,
        const std::unordered_set<int>& deadInstructions);
    
    static bool shouldKeepInstruction(const ircpp::IRInstruction& instruction, const std::unordered_set<int>& deadInstructions);
};

#endif //IRCPP_IR_OPTIMIZER_H
