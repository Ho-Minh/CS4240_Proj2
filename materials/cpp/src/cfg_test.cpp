#include "ir.hpp"
#include "reaching_def.hpp"
#include <iostream>
#include <fstream>
#include <string>

using namespace ircpp;

void testCFGConstruction(const std::string& irFile) {
    std::cout << "=== Testing CFG Construction for: " << irFile << " ===" << std::endl;
    
    // Read the IR program
    std::cout << "Step 1: Reading IR file..." << std::endl;
    IRReader reader;
    IRProgram program;
    try {
        program = reader.parseIRFile(irFile);
        std::cout << "Successfully loaded IR program with " << program.functions.size() << " functions" << std::endl;
    } catch (const IRException& e) {
        std::cerr << "Error loading IR file: " << e.what() << std::endl;
        return;
    }
    
    // Build CFGs for all functions
    std::cout << "Step 2: Building CFGs..." << std::endl;
    std::vector<ControlFlowGraph> functionCfgs;
    for (const auto& function : program.functions) {
        std::cout << "\n--- Function: " << function->name << " ---" << std::endl;
        
        // Build CFG
        std::cout << "Building CFG for function: " << function->name << std::endl;
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*function);
        std::cout << "CFG built successfully" << std::endl;
        functionCfgs.push_back(cfg);
        
        // Print CFG structure
        std::cout << "CFG Structure:" << std::endl;
        for (const auto& [blockName, block] : cfg.blocks) {
            std::cout << "  Block " << blockName << " -> ";
            for (const auto& successor : block->successors) {
                std::cout << successor << " ";
            }
            std::cout << std::endl;
        }
        
        // Print detailed CFG
        std::cout << "\nDetailed CFG:" << std::endl;
        CFGBuilder::printCFG(cfg, std::cout);
        
        // Generate dot file
        std::string dotFileName = function->name + "_cfg.dot";
        std::ofstream dotFile(dotFileName);
        if (dotFile.is_open()) {
            CFGBuilder::printCFGDot(cfg, dotFile);
            dotFile.close();
            std::cout << "\nDot file generated: " << dotFileName << std::endl;
            std::cout << "To visualize, run: dot -Tpng " << dotFileName << " -o " << function->name << "_cfg.png" << std::endl;
        } else {
            std::cerr << "Failed to create dot file: " << dotFileName << std::endl;
        }
    }
    
    // Test reaching definitions
    std::cout << "\n=== Testing Reaching Definitions ===" << std::endl;
    try {
        std::cout << "Computing reaching definitions..." << std::endl;
        ReachingDefOut reachingDefs = computeReachingDefs(functionCfgs);
        std::cout << "Reaching definitions computed successfully" << std::endl;
        
        for (size_t i = 0; i < functionCfgs.size(); ++i) {
            const auto& cfg = functionCfgs[i];
            const auto& funcReachingDefs = reachingDefs[i];
            
            std::cout << "\n--- Reaching Definitions for " << program.functions[i]->name << " ---" << std::endl;
            for (const auto& [blockName, block] : cfg.blocks) {
                std::cout << "Block " << blockName << ":" << std::endl;
                std::cout << "  IN:  ";
                for (const auto& def : funcReachingDefs.at(blockName).in) {
                    std::cout << def << " ";
                }
                std::cout << std::endl;
                std::cout << "  OUT: ";
                for (const auto& def : funcReachingDefs.at(blockName).out) {
                    std::cout << def << " ";
                }
                std::cout << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error computing reaching definitions: " << e.what() << std::endl;
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <ir_file>" << std::endl;
    std::cout << "  ir_file: Path to the IR file to test" << std::endl;
    std::cout << std::endl;
    std::cout << "This program will:" << std::endl;
    std::cout << "  1. Load and parse the IR file" << std::endl;
    std::cout << "  2. Build CFGs for all functions" << std::endl;
    std::cout << "  3. Print CFG structure and details" << std::endl;
    std::cout << "  4. Generate dot files for visualization" << std::endl;
    std::cout << "  5. Compute and display reaching definitions" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string irFile = argv[1];
    
    // Add more debug output
    std::cout << "Starting test with file: " << irFile << std::endl;
    std::cout << "File exists check..." << std::endl;
    
    std::ifstream testFile(irFile);
    if (!testFile.good()) {
        std::cerr << "Error: Cannot open file " << irFile << std::endl;
        return 1;
    }
    testFile.close();
    std::cout << "File exists and is readable" << std::endl;
    
    testCFGConstruction(irFile);
    
    return 0;
}
