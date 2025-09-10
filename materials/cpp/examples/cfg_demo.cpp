#include "ir.hpp"
#include <iostream>
#include <fstream>

using namespace ircpp;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.ir> [output.dot]" << std::endl;
        return 1;
    }

    try {
        // Parse the IR file
        IRReader reader;
        IRProgram program = reader.parseIRFile(argv[1]);

        std::cout << "Parsed " << program.functions.size() << " functions:" << std::endl;

        // Build CFG for each function
        for (const auto& function : program.functions) {
            std::cout << "\n" << std::string(50, '=') << std::endl;
            std::cout << "Function: " << function->name << std::endl;
            std::cout << std::string(50, '=') << std::endl;

            // Build the control flow graph
            ControlFlowGraph cfg = CFGBuilder::buildCFG(*function);

            // Print CFG in text format
            CFGBuilder::printCFG(cfg, std::cout);

            // Generate DOT file if output file specified
            if (argc >= 3) {
                std::string outputFile = argv[2];
                if (outputFile.find(".dot") == std::string::npos) {
                    outputFile += "_" + function->name + ".dot";
                }
                
                std::ofstream dotFile(outputFile);
                if (dotFile.is_open()) {
                    CFGBuilder::printCFGDot(cfg, dotFile);
                    dotFile.close();
                    std::cout << "DOT file written to: " << outputFile << std::endl;
                    std::cout << "To visualize, run: dot -Tpng " << outputFile << " -o " 
                              << outputFile.substr(0, outputFile.find(".dot")) << ".png" << std::endl;
                } else {
                    std::cerr << "Error: Could not open output file " << outputFile << std::endl;
                }
            }
        }

    } catch (const IRException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
