#include "ir.hpp"
#include <iostream>

using namespace ircpp;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.ir>\n";
        return 1;
    }

    try {
        IRInterpreter interpreter(argv[1]);
        interpreter.run();
        
        const auto& stats = interpreter.getStats();
        std::cout << "\n=== Execution Statistics ===" << std::endl;
        std::cout << "Total instructions: " << stats.totalInstructionCount << std::endl;
        std::cout << "Non-label instructions: " << stats.getNonLabelInstructionCount() << std::endl;
        
        std::cout << "\nInstruction counts by type:" << std::endl;
        for (const auto& [opcode, count] : stats.instructionCounts) {
            if (count > 0) {
                std::cout << "  " << static_cast<int>(opcode) << ": " << count << std::endl;
            }
        }
        
    } catch (const IRException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
