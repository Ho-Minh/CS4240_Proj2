#include "ir.hpp"
#include <iostream>
#include <fstream>
#include <set>

using namespace ircpp;

// Helper function to get display name for a block
static std::string getBlockDisplayName(const std::shared_ptr<BasicBlock>& block) {
    // If the block starts with a label instruction, use the label name
    if (!block->instructions.empty() && 
        block->instructions[0]->opCode == IRInstruction::OpCode::LABEL) {
        auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(block->instructions[0]->operands[0]);
        return labelOp->getName();
    }
    
    // For other blocks, create a more descriptive name based on content
    if (!block->instructions.empty()) {
        const auto& lastInst = block->instructions.back();
        switch (lastInst->opCode) {
            case IRInstruction::OpCode::GOTO: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                return "goto_" + labelOp->getName();
            }
            case IRInstruction::OpCode::BREQ: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                return "breq_" + labelOp->getName();
            }
            case IRInstruction::OpCode::BRNEQ: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                return "brneq_" + labelOp->getName();
            }
            case IRInstruction::OpCode::BRLT: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                return "brlt_" + labelOp->getName();
            }
            case IRInstruction::OpCode::BRGT: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                return "brgt_" + labelOp->getName();
            }
            case IRInstruction::OpCode::BRGEQ: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                return "brgeq_" + labelOp->getName();
            }
            case IRInstruction::OpCode::RETURN:
                return "return_block";
            default:
                break;
        }
    }
    
    // For entry blocks or other cases, use a more descriptive name
    if (block->id.find("B") == 0) {
        return "block_" + block->id.substr(1); // block_5, block_8, etc.
    }
    
    return block->id; // Fallback to internal ID
}

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

        // Build CFG for each function and collect them
        std::vector<ControlFlowGraph> functionCFGs;
        std::vector<std::string> functionNames;
        
        for (const auto& function : program.functions) {
            std::cout << "\n" << std::string(50, '=') << std::endl;
            std::cout << "Function: " << function->name << std::endl;
            std::cout << std::string(50, '=') << std::endl;

            // Build the control flow graph
            ControlFlowGraph cfg = CFGBuilder::buildCFG(*function);
            functionCFGs.push_back(cfg);
            functionNames.push_back(function->name);

            // Print CFG in text format
            CFGBuilder::printCFG(cfg, std::cout);
        }

        // Generate combined DOT file if output file specified
        if (argc >= 3) {
            std::string outputFile = argv[2];
            if (outputFile.find(".dot") == std::string::npos) {
                outputFile += ".dot";
            }
            
            std::ofstream dotFile(outputFile);
            if (dotFile.is_open()) {
                // Write combined DOT file with all functions
                dotFile << "digraph CFG {" << std::endl;
                dotFile << "  rankdir=TB;" << std::endl;
                dotFile << "  node [shape=box, style=filled, fillcolor=lightblue];" << std::endl;
                dotFile << "  subgraph cluster_legend {" << std::endl;
                dotFile << "    label=\"Functions\";" << std::endl;
                dotFile << "    style=dashed;" << std::endl;
                for (size_t i = 0; i < functionNames.size(); ++i) {
                    dotFile << "    \"" << functionNames[i] << "_entry\" [label=\"" << functionNames[i] << " (entry)\", fillcolor=lightgreen];" << std::endl;
                }
                dotFile << "  }" << std::endl;
                
                // Add all blocks from all functions with function prefix
                for (size_t funcIdx = 0; funcIdx < functionCFGs.size(); ++funcIdx) {
                    const auto& cfg = functionCFGs[funcIdx];
                    const std::string& funcName = functionNames[funcIdx];
                    
                    for (const auto& [blockId, block] : cfg.blocks) {
                        std::string prefixedBlockId = funcName + "_" + blockId;
                        std::string displayName = getBlockDisplayName(block);
                        dotFile << "  \"" << prefixedBlockId << "\" [label=\"" << funcName << "::" << displayName;
                        if (!block->instructions.empty()) {
                            dotFile << "\\n";
                            for (const auto& inst : block->instructions) {
                                switch (inst->opCode) {
                                    case IRInstruction::OpCode::ASSIGN: dotFile << "assign"; break;
                                    case IRInstruction::OpCode::ADD: dotFile << "add"; break;
                                    case IRInstruction::OpCode::SUB: dotFile << "sub"; break;
                                    case IRInstruction::OpCode::MULT: dotFile << "mult"; break;
                                    case IRInstruction::OpCode::DIV: dotFile << "div"; break;
                                    case IRInstruction::OpCode::AND: dotFile << "and"; break;
                                    case IRInstruction::OpCode::OR: dotFile << "or"; break;
                                    case IRInstruction::OpCode::GOTO: dotFile << "goto"; break;
                                    case IRInstruction::OpCode::BREQ: dotFile << "breq"; break;
                                    case IRInstruction::OpCode::BRNEQ: dotFile << "brneq"; break;
                                    case IRInstruction::OpCode::BRLT: dotFile << "brlt"; break;
                                    case IRInstruction::OpCode::BRGT: dotFile << "brgt"; break;
                                    case IRInstruction::OpCode::BRGEQ: dotFile << "brgeq"; break;
                                    case IRInstruction::OpCode::RETURN: dotFile << "return"; break;
                                    case IRInstruction::OpCode::CALL: dotFile << "call"; break;
                                    case IRInstruction::OpCode::CALLR: dotFile << "callr"; break;
                                    case IRInstruction::OpCode::ARRAY_STORE: dotFile << "array_store"; break;
                                    case IRInstruction::OpCode::ARRAY_LOAD: dotFile << "array_load"; break;
                                    case IRInstruction::OpCode::LABEL: dotFile << "label"; break;
                                }
                                dotFile << "\\n";
                            }
                        }
                        dotFile << "\"];" << std::endl;
                    }
                }
                
                // Add edges within functions and between functions
                for (size_t funcIdx = 0; funcIdx < functionCFGs.size(); ++funcIdx) {
                    const auto& cfg = functionCFGs[funcIdx];
                    const std::string& funcName = functionNames[funcIdx];
                    
                    for (const auto& [blockId, block] : cfg.blocks) {
                        std::string prefixedBlockId = funcName + "_" + blockId;
                        
                        for (const auto& successor : block->successors) {
                            std::string prefixedSuccessor = funcName + "_" + successor;
                            dotFile << "  \"" << prefixedBlockId << "\" -> \"" << prefixedSuccessor << "\";" << std::endl;
                        }
                        
                        // Check for function calls and add inter-function edges
                        std::set<std::string> calledFunctions; // Track unique function calls to avoid duplicates
                        for (const auto& inst : block->instructions) {
                            if (inst->opCode == IRInstruction::OpCode::CALL || inst->opCode == IRInstruction::OpCode::CALLR) {
                                if (!inst->operands.empty()) {
                                    auto funcOp = std::dynamic_pointer_cast<IRFunctionOperand>(inst->operands[0]);
                                    if (funcOp) {
                                        std::string calledFunc = funcOp->getName();
                                        calledFunctions.insert(calledFunc);
                                    }
                                }
                            }
                        }
                        
                        // Create one edge per unique function call
                        for (const auto& calledFunc : calledFunctions) {
                            // Find the called function's entry block
                            for (size_t calledIdx = 0; calledIdx < functionNames.size(); ++calledIdx) {
                                if (functionNames[calledIdx] == calledFunc) {
                                    std::string calledEntry = calledFunc + "_" + functionCFGs[calledIdx].entryBlock;
                                    dotFile << "  \"" << prefixedBlockId << "\" -> \"" << calledEntry << "\" [style=dashed, color=red];" << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                }
                
                dotFile << "}" << std::endl;
                dotFile.close();
                std::cout << "Combined DOT file written to: " << outputFile << std::endl;
                std::cout << "To visualize, run: dot -Tpng " << outputFile << " -o " 
                          << outputFile.substr(0, outputFile.find(".dot")) << ".png" << std::endl;
            } else {
                std::cerr << "Error: Could not open output file " << outputFile << std::endl;
            }
        }

    } catch (const IRException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
