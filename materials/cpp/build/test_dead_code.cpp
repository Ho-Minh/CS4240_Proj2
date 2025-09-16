#include "ir.hpp"
#include "reaching_def.hpp"
#include "dead_code.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace ircpp;

// Helper function to print dead code analysis results
void printDeadCodeAnalysis(const DeadCodeAnalysis& analysis, const std::string& functionName) {
    std::cout << "Dead Code Analysis for " << functionName << ":" << std::endl;
    
    std::cout << "  Unreachable instructions: ";
    if (analysis.unreachableInstructions.empty()) {
        std::cout << "none";
    } else {
        bool first = true;
        for (int line : analysis.unreachableInstructions) {
            if (!first) std::cout << ", ";
            std::cout << line;
            first = false;
        }
    }
    std::cout << std::endl;
    
    std::cout << "  Unused assignments: ";
    if (analysis.unusedAssignments.empty()) {
        std::cout << "none";
    } else {
        bool first = true;
        for (int line : analysis.unusedAssignments) {
            if (!first) std::cout << ", ";
            std::cout << line;
            first = false;
        }
    }
    std::cout << std::endl;
    
    std::cout << "  Total dead instructions: " << analysis.deadInstructions.size() << std::endl;
}

void testUnreachableCode() {
    std::cout << "=== Testing Unreachable Code Detection ===" << std::endl;
    
    // Create a program with unreachable code after an unconditional goto
    std::string irContent = R"(#start_function
int main():
int-list: a, b, c
float-list:
    assign, a, 10
    assign, b, 20
    goto, end
    assign, c, 30    # This should be unreachable
    call, puti, c    # This should also be unreachable
end:
    call, puti, a
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_unreachable.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_unreachable.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        std::vector<ControlFlowGraph> cfgs = {cfg};
        DeadCodeResult result = analyzeDeadCode(cfgs);
        
        assert(result.functionResults.size() == 1);
        auto& analysis = result.functionResults[0]["analysis"];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printDeadCodeAnalysis(analysis, "main");
        
        // Should find some unreachable instructions
        assert(!analysis.unreachableInstructions.empty());
        
        std::cout << "✓ Unreachable code detection test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testUnusedAssignments() {
    std::cout << "=== Testing Unused Assignment Detection ===" << std::endl;
    
    // Create a program with unused variable assignments
    std::string irContent = R"(#start_function
int main():
int-list: a, b, c, unused1, unused2
float-list:
    assign, a, 10
    assign, b, 20
    add, c, a, b
    assign, unused1, 100    # This should be detected as unused
    assign, unused2, 200    # This should also be unused
    call, puti, c
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_unused.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_unused.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        std::vector<ControlFlowGraph> cfgs = {cfg};
        DeadCodeResult result = analyzeDeadCode(cfgs);
        
        assert(result.functionResults.size() == 1);
        auto& analysis = result.functionResults[0]["analysis"];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printDeadCodeAnalysis(analysis, "main");
        
        // Should find some unused assignments
        assert(!analysis.unusedAssignments.empty());
        
        std::cout << "✓ Unused assignment detection test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testBranchingDeadCode() {
    std::cout << "=== Testing Dead Code in Branches ===" << std::endl;
    
    // Create a program with dead code in different branches
    std::string irContent = R"(#start_function
int main():
int-list: x, y, dead1, dead2
float-list:
    assign, x, 5
    brgt, positive, x, 0
    assign, y, -1
    assign, dead1, 999    # Dead code in false branch
    goto, end
positive:
    assign, y, 1
    assign, dead2, 888    # Dead code in true branch
end:
    call, puti, y
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_branching_dead.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_branching_dead.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        std::vector<ControlFlowGraph> cfgs = {cfg};
        DeadCodeResult result = analyzeDeadCode(cfgs);
        
        assert(result.functionResults.size() == 1);
        auto& analysis = result.functionResults[0]["analysis"];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printDeadCodeAnalysis(analysis, "main");
        
        // Should find some unused assignments
        assert(!analysis.unusedAssignments.empty());
        
        std::cout << "✓ Branching dead code test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testLoopDeadCode() {
    std::cout << "=== Testing Dead Code in Loops ===" << std::endl;
    
    // Create a program with a loop and some dead code
    std::string irContent = R"(#start_function
int main():
int-list: i, sum, dead_var
float-list:
    assign, i, 0
    assign, sum, 0
    assign, dead_var, 123    # This should be unused
loop:
    brgt, done, i, 5
    add, sum, sum, i
    add, i, i, 1
    goto, loop
done:
    call, puti, sum
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_loop_dead.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_loop_dead.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        std::vector<ControlFlowGraph> cfgs = {cfg};
        DeadCodeResult result = analyzeDeadCode(cfgs);
        
        assert(result.functionResults.size() == 1);
        auto& analysis = result.functionResults[0]["analysis"];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printDeadCodeAnalysis(analysis, "main");
        
        // Should find some unused assignments
        assert(!analysis.unusedAssignments.empty());
        
        std::cout << "✓ Loop dead code test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testMultipleFunctionsDeadCode() {
    std::cout << "=== Testing Dead Code in Multiple Functions ===" << std::endl;
    
    // Create a program with multiple functions, some with dead code
    std::string irContent = R"(#start_function
int helper(int x):
int-list: result, unused
float-list:
    add, result, x, 1
    assign, unused, 999    # Dead code
    return, result
#end_function

#start_function
int main():
int-list: a, b, dead_var
float-list:
    assign, a, 10
    callr, b, helper, a
    assign, dead_var, 888    # Dead code
    call, puti, b
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_multiple_functions_dead.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_multiple_functions_dead.ir");
        
        std::vector<ControlFlowGraph> cfgs;
        for (const auto& func : program.functions) {
            cfgs.push_back(CFGBuilder::buildCFG(*func));
        }
        
        DeadCodeResult result = analyzeDeadCode(cfgs);
        
        assert(result.functionResults.size() == 2);
        
        std::cout << "Function 1 (helper) CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfgs[0].blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << "Function 2 (main) CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfgs[1].blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printDeadCodeAnalysis(result.functionResults[0]["analysis"], "helper");
        printDeadCodeAnalysis(result.functionResults[1]["analysis"], "main");
        
        // Should find dead code in both functions
        assert(!result.functionResults[0]["analysis"].unusedAssignments.empty());
        assert(!result.functionResults[1]["analysis"].unusedAssignments.empty());
        
        std::cout << "✓ Multiple functions dead code test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testRealProgramDeadCode() {
    std::cout << "=== Testing Dead Code in Real Program ===" << std::endl;
    
    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("../../example/example.ir");
        
        std::vector<ControlFlowGraph> cfgs;
        for (const auto& func : program.functions) {
            cfgs.push_back(CFGBuilder::buildCFG(*func));
        }
        
        DeadCodeResult result = analyzeDeadCode(cfgs);
        
        std::cout << "Real program dead code analysis:" << std::endl;
        std::cout << "Total functions: " << result.functionResults.size() << std::endl;
        std::cout << "Total dead instructions: " << result.totalDeadInstructions << std::endl;
        std::cout << "Total unreachable instructions: " << result.totalUnreachableInstructions << std::endl;
        std::cout << "Total unused assignments: " << result.totalUnusedAssignments << std::endl;
        
        for (size_t i = 0; i < result.functionResults.size(); ++i) {
            const auto& funcResult = result.functionResults[i]["analysis"];
            std::cout << "Function " << i << ": " << funcResult.deadInstructions.size() << " dead instructions" << std::endl;
        }
        
        std::cout << "✓ Real program dead code test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Dead Code Elimination Testing Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testUnreachableCode();
        testUnusedAssignments();
        testBranchingDeadCode();
        testLoopDeadCode();
        testMultipleFunctionsDeadCode();
        testRealProgramDeadCode();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All dead code elimination tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Clean up temporary files
    std::remove("temp_unreachable.ir");
    std::remove("temp_unused.ir");
    std::remove("temp_branching_dead.ir");
    std::remove("temp_loop_dead.ir");
    std::remove("temp_multiple_functions_dead.ir");
    
    return 0;
}
