#include "ir.hpp"
#include "reaching_def.hpp"
#include "dead_code.hpp"
#include "ir_optimizer.hpp"
#include <iostream>
#include <fstream>

using namespace ircpp;

void optimizeAndCompare(const std::string& inputFile, const std::string& outputFile) {
    std::cout << "========================================" << std::endl;
    std::cout << "IR Optimization: " << inputFile << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // Parse original IR file
        IRReader reader;
        IRProgram originalProgram = reader.parseIRFile(inputFile);
        
        std::cout << "Original program loaded successfully" << std::endl;
        
        // Perform optimization
        std::cout << "\nPerforming dead code elimination..." << std::endl;
        auto optimizedProgram = IROptimizer::optimizeProgram(originalProgram);
        
        // Write optimized program to file
        std::cout << "\nWriting optimized program to: " << outputFile << std::endl;
        IROptimizer::writeOptimizedProgram(*optimizedProgram, outputFile);
        
        // Show detailed analysis
        std::cout << "\n=== Detailed Analysis ===" << std::endl;
        
        // Build CFGs for analysis
        std::vector<ControlFlowGraph> cfgs;
        for (const auto& func : originalProgram.functions) {
            cfgs.push_back(CFGBuilder::buildCFG(*func));
        }
        
        // Perform dead code analysis
        DeadCodeResult deadCodeResult = analyzeDeadCode(cfgs);
        
        for (size_t funcIdx = 0; funcIdx < originalProgram.functions.size(); ++funcIdx) {
            const auto& func = originalProgram.functions[funcIdx];
            const auto& deadCodeAnalysis = deadCodeResult.functionResults[funcIdx]["analysis"];
            
            std::cout << "\nFunction: " << func->name << std::endl;
            std::cout << "  Original instructions: " << func->instructions.size() << std::endl;
            std::cout << "  Dead instructions found: " << deadCodeAnalysis.deadInstructions.size() << std::endl;
        }
        
        std::cout << "\n=== Optimization Complete ===" << std::endl;
        std::cout << "Optimized IR file saved as: " << outputFile << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR parsing IR file: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "IR Optimizer - Dead Code Elimination" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_ir_file> <output_ir_file>" << std::endl;
        std::cout << "Example: " << argv[0] << " ../../example/example.ir optimized_example.ir" << std::endl;
        std::cout << "Example: " << argv[0] << " ../../../public_test_cases/quicksort/quicksort.ir optimized_quicksort.ir" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    
    optimizeAndCompare(inputFile, outputFile);
    
    return 0;
}
