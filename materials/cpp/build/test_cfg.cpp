#include "ir.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace ircpp;

void testSimpleCFG() {
    std::cout << "=== Testing Simple CFG ===" << std::endl;
    
    // Create a simple IR program with basic control flow
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, c
float-list:
    assign, a, 10
    assign, b, 5
    brgt, greater, a, b
    assign, c, 0
    goto, end
greater:
    assign, c, 1
end:
    call, puti, c
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_simple_cfg.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_simple_cfg.ir");
        
        assert(program.functions.size() == 1);
        auto& func = program.functions[0];
        
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Verify basic structure
        assert(cfg.blocks.size() >= 3); // Should have at least entry, greater, end blocks
        assert(!cfg.entryBlock.empty());
        assert(!cfg.exitBlocks.empty());
        
        std::cout << "✓ Simple CFG created successfully" << std::endl;
        std::cout << "  Entry block: " << cfg.entryBlock << std::endl;
        std::cout << "  Number of blocks: " << cfg.blocks.size() << std::endl;
        std::cout << "  Exit blocks: " << cfg.exitBlocks.size() << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testLoopCFG() {
    std::cout << "=== Testing Loop CFG ===" << std::endl;
    
    // Create a program with a loop
    std::string irContent = R"(
#start_function
int main():
int-list: i, sum
float-list:
    assign, i, 0
    assign, sum, 0
loop:
    brgt, done, i, 5
    add, sum, sum, i
    add, i, i, 1
    goto, loop
done:
    call, puti, sum
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_loop_cfg.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_loop_cfg.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Verify we have the expected blocks and control flow
        bool foundLoop = false;
        bool foundDone = false;
        bool foundBackEdge = false;
        std::string loopBlockId, doneBlockId;
        
        // Find blocks that contain the loop and done labels
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst->opCode == IRInstruction::OpCode::LABEL) {
                    auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
                    if (labelOp->getName() == "loop") {
                        foundLoop = true;
                        loopBlockId = blockId;
                        std::cout << "  Found loop block: " << blockId << std::endl;
                    }
                    if (labelOp->getName() == "done") {
                        foundDone = true;
                        doneBlockId = blockId;
                        std::cout << "  Found done block: " << blockId << std::endl;
                    }
                }
            }
        }
        
        // Check if any block has the loop block as a successor (back edge)
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& successor : block->successors) {
                if (successor == loopBlockId) {
                    foundBackEdge = true;
                    std::cout << "  Found back edge from " << blockId << " to " << loopBlockId << std::endl;
                    break;
                }
            }
        }
        
        assert(foundLoop);
        assert(foundDone);
        assert(foundBackEdge);
        std::cout << "✓ Loop CFG created successfully with back edge" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testComplexCFG() {
    std::cout << "=== Testing Complex CFG ===" << std::endl;
    
    // Create a complex program with multiple branches
    std::string irContent = R"(
#start_function
int main():
int-list: x, y, z
float-list:
    callr, x, geti
    brlt, negative, x, 0
    brgt, positive, x, 0
    assign, z, 0
    goto, end
negative:
    assign, z, -1
    goto, end
positive:
    assign, z, 1
end:
    call, puti, z
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_complex_cfg.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_complex_cfg.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Debug: Print all blocks
        std::cout << "  All blocks in CFG:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "    Block: " << blockId << " -> [";
            for (size_t i = 0; i < block->successors.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << block->successors[i];
            }
            std::cout << "]" << std::endl;
        }
        
        // Find blocks that contain the expected labels
        std::string negativeBlockId, positiveBlockId, endBlockId;
        bool foundNegative = false, foundPositive = false, foundEnd = false;
        
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst->opCode == IRInstruction::OpCode::LABEL) {
                    auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
                    if (labelOp->getName() == "negative") {
                        foundNegative = true;
                        negativeBlockId = blockId;
                        std::cout << "  Found negative block: " << blockId << std::endl;
                    }
                    if (labelOp->getName() == "positive") {
                        foundPositive = true;
                        positiveBlockId = blockId;
                        std::cout << "  Found positive block: " << blockId << std::endl;
                    }
                    if (labelOp->getName() == "end") {
                        foundEnd = true;
                        endBlockId = blockId;
                        std::cout << "  Found end block: " << blockId << std::endl;
                    }
                }
            }
        }
        
        assert(foundNegative);
        assert(foundPositive);
        assert(foundEnd);
        
        // Verify control flow - check if any block has the expected edges
        bool hasNegativeEdge = false, hasPositiveEdge = false;
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& successor : block->successors) {
                if (successor == negativeBlockId) {
                    hasNegativeEdge = true;
                    std::cout << "  Found edge from " << blockId << " to " << negativeBlockId << std::endl;
                }
                if (successor == positiveBlockId) {
                    hasPositiveEdge = true;
                    std::cout << "  Found edge from " << blockId << " to " << positiveBlockId << std::endl;
                }
            }
        }
        
        assert(hasNegativeEdge);
        assert(hasPositiveEdge);
        
        std::cout << "✓ Complex CFG created successfully" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testRealIRFileCFG() {
    std::cout << "=== Testing Real IR File CFG ===" << std::endl;
    
    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("../../example/example.ir");
        
        for (const auto& func : program.functions) {
            std::cout << "Building CFG for function: " << func->name << std::endl;
            
            ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
            
            std::cout << "  Entry block: " << cfg.entryBlock << std::endl;
            std::cout << "  Number of blocks: " << cfg.blocks.size() << std::endl;
            std::cout << "  Exit blocks: " << cfg.exitBlocks.size() << std::endl;
            
            // Verify basic properties
            assert(!cfg.entryBlock.empty());
            assert(cfg.blocks.count(cfg.entryBlock));
            assert(!cfg.exitBlocks.empty());
            
            // Print CFG structure
            std::cout << "  Block structure:" << std::endl;
            for (const auto& [blockId, block] : cfg.blocks) {
                std::cout << "    " << blockId << " -> [";
                for (size_t i = 0; i < block->successors.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << block->successors[i];
                }
                std::cout << "]" << std::endl;
            }
        }
        
        std::cout << "✓ Real IR file CFG created successfully" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testCFGOutput() {
    std::cout << "=== Testing CFG Output ===" << std::endl;
    
    std::string irContent = R"(
#start_function
int test():
int-list: a, b
float-list:
    assign, a, 1
    brgt, label1, a, 0
    assign, b, 0
    goto, end
label1:
    assign, b, 1
end:
    return, b
#end_function
)";

    std::ofstream tempFile("temp_cfg_output.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_cfg_output.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Test text output
        std::stringstream textOutput;
        CFGBuilder::printCFG(cfg, textOutput);
        std::string text = textOutput.str();
        
        assert(text.find("Entry block:") != std::string::npos);
        assert(text.find("Block:") != std::string::npos);
        
        // Test DOT output
        std::stringstream dotOutput;
        CFGBuilder::printCFGDot(cfg, dotOutput);
        std::string dot = dotOutput.str();
        
        assert(dot.find("digraph CFG") != std::string::npos);
        assert(dot.find("->") != std::string::npos);
        
        std::cout << "✓ CFG output formats work correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Control Flow Graph Testing Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testSimpleCFG();
        testLoopCFG();
        testComplexCFG();
        testRealIRFileCFG();
        testCFGOutput();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All CFG tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
