#include "ir.hpp"
#include <iostream>
#include <fstream>

using namespace ircpp;

int main() {
    std::cout << "=== Simple Loop CFG Test ===" << std::endl;
    
    // Create a very simple loop
    std::string irContent = R"(
#start_function
int main():
int-list: i
float-list:
loop:
    add, i, i, 1
    goto, loop
#end_function
)";

    std::ofstream tempFile("temp_simple_loop.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_simple_loop.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << " -> [";
            for (size_t i = 0; i < block->successors.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << block->successors[i];
            }
            std::cout << "]" << std::endl;
        }
        
        // Check if loop block has itself as successor
        if (cfg.blocks.count("loop")) {
            auto loopBlock = cfg.blocks["loop"];
            bool hasSelfEdge = false;
            for (const auto& successor : loopBlock->successors) {
                if (successor == "loop") {
                    hasSelfEdge = true;
                    break;
                }
            }
            std::cout << "Loop block has self-edge: " << (hasSelfEdge ? "YES" : "NO") << std::endl;
        }
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
