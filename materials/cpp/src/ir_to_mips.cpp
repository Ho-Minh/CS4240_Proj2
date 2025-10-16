#include <iostream>
#include <string>
#include "ir.hpp"
#include "instruction_selector.hpp"

int main(int argc, char* argv[]) {
    // TODO: Implement command line argument parsing
    // Expected usage: ./ir_to_mips input.ir output.s
    // 
    // Steps:
    // 1. Check for correct number of arguments
    // 2. Parse input IR file path
    // 3. Parse output MIPS assembly file path
    // 4. Handle error cases and print usage
    
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.ir> <output.s>" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    
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
        
        // Create instruction selector
        ircpp::IRToMIPSSelector selector;
        
        // Convert to MIPS
        std::vector<ircpp::MIPSInstruction> mipsInstructions = selector.selectProgram(program);
        
        // Generate and write assembly
        std::string assembly = selector.generateAssembly(mipsInstructions);
        selector.writeAssemblyFile(outputFile, mipsInstructions);
        
        std::cout << "Successfully converted " << inputFile << " to " << outputFile << std::endl;
        
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

// TODO: Implement helper functions for testing
// Add functions for testing individual components:
// - testArithmeticSelector()
// - testBranchSelector()
// - testMemorySelector()
// - testCallSelector()
// - testRegisterManager()

// Example test function structure:
/*
void testArithmeticSelector() {
    // TODO: Create test IR instructions
    // TODO: Create selection context
    // TODO: Call selector
    // TODO: Verify MIPS output
    // TODO: Print results
}
*/
