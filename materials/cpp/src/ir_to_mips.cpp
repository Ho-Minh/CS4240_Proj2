#include <bits/stdc++.h>
#include "ir.hpp"
#include "instruction_selector.hpp"
#include "mips_instructions.hpp"
#include "register_manager.hpp"

int main(int argc, char* argv[]) {
    // Usage:
    //   ./ir_to_mips <input.ir> <output.s> [--naive | --greedy]
    // Default mode is --naive
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input.ir> <output.s> [--naive|--greedy]" << std::endl;
        return 1;
    }
    
    std::string inputFile(argv[1]);
    std::string outputFile(argv[2]);
    ircpp::IRToMIPSSelector::AllocMode mode = ircpp::IRToMIPSSelector::AllocMode::Naive;
    if (argc == 4) {
        std::string flag(argv[3]);
        if (flag == "--naive") mode = ircpp::IRToMIPSSelector::AllocMode::Naive;
        else if (flag == "--greedy") mode = ircpp::IRToMIPSSelector::AllocMode::Greedy;
        else {
            std::cerr << "Unknown flag: " << flag << std::endl;
            std::cerr << "Allowed: --naive, --greedy" << std::endl;
            return 1;
        }
    }
    
    try {
        // TODO: Implement IR to MIPS conversion
        // Steps:
        // 1. Parse IR file using IRReader
        // 2. Create instruction selector
        // 3. Convert IR program to MIPS instructions
        // 4. Generate assembly text
        // 5. Write to output file
        
        // Parse IR file
        ircpp::IRReader reader;
        ircpp::IRProgram program = reader.parseIRFile(inputFile);
        
        // Create instruction selector with desired allocation mode
        ircpp::IRToMIPSSelector selector(mode);
        
        // Convert to MIPS
        std::vector<ircpp::MIPSInstruction> mipsInstructions = selector.selectProgram(program);
        
        // Generate and write assembly
        std::string assembly = selector.generateAssembly(mipsInstructions);
        selector.writeAssemblyFile(outputFile, mipsInstructions);
        
        // Conversion complete
        
    } catch (const ircpp::IRException& e) {
        // TODO: Implement error handling
        // Handle IR parsing errors
        std::cerr << "IR Error: " << e.what() << std::endl;
        return 1;
        
    } catch (const std::exception& e) {
        // TODO: Implement general error handling
        // Handle other errors (file I/O, etc.)
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}