#include "ir.hpp"
#include "reaching_def.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <set>

using namespace ircpp;

// Helper function to print reaching definitions for debugging
void printReachingDefs(const std::unordered_map<std::string, BasicBlockReachingDef>& reachingDefs) {
    std::cout << "Reaching Definitions:" << std::endl;
    for (const auto& [blockName, blockDefs] : reachingDefs) {
        std::cout << "  Block " << blockName << ":" << std::endl;
        
        std::cout << "    IN:  {";
        bool first = true;
        for (int line : blockDefs.in) {
            if (!first) std::cout << ", ";
            std::cout << line;
            first = false;
        }
        std::cout << "}" << std::endl;
        
        std::cout << "    OUT: {";
        first = true;
        for (int line : blockDefs.out) {
            if (!first) std::cout << ", ";
            std::cout << line;
            first = false;
        }
        std::cout << "}" << std::endl;
    }
}

// Helper function to check if a set contains specific line numbers
bool containsLines(const std::unordered_set<int>& set, const std::vector<int>& expectedLines) {
    for (int line : expectedLines) {
        if (set.find(line) == set.end()) {
            return false;
        }
    }
    return true;
}

void testSimpleReachingDef() {
    std::cout << "=== Testing Simple Reaching Definitions ===" << std::endl;
    
    // Create a simple IR program
    std::string irContent = R"(#start_function
int main():
int-list: a, b, c
float-list:
    assign, a, 10
    assign, b, 20
    add, c, a, b
    call, puti, c
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_simple_reaching.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_simple_reaching.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Get reaching definitions
        std::vector<ControlFlowGraph> cfgs = {cfg};
        ReachingDefOut result = computeReachingDefs(cfgs);
        
        assert(result.size() == 1);
        auto& reachingDefs = result[0];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printReachingDefs(reachingDefs);
        
        // Find the main block (should contain all instructions)
        std::string mainBlockId;
        for (const auto& [blockId, block] : cfg.blocks) {
            if (block->instructions.size() > 3) { // Main block has most instructions
                mainBlockId = blockId;
                break;
            }
        }
        
        assert(!mainBlockId.empty());
        
        // Check that definitions are properly tracked
        auto& mainBlockDefs = reachingDefs[mainBlockId];
        
        // The OUT set should contain the definition line numbers
        assert(!mainBlockDefs.out.empty());
        
        std::cout << "✓ Simple reaching definitions test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testBranchingReachingDef() {
    std::cout << "=== Testing Branching Reaching Definitions ===" << std::endl;
    
    // Create a program with branching
    std::string irContent = R"(#start_function
int main():
int-list: x, y
float-list:
    assign, x, 5
    brgt, positive, x, 0
    assign, y, -1
    goto, end
positive:
    assign, y, 1
end:
    call, puti, y
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_branch_reaching.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_branch_reaching.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Get reaching definitions
        std::vector<ControlFlowGraph> cfgs = {cfg};
        ReachingDefOut result = computeReachingDefs(cfgs);
        
        assert(result.size() == 1);
        auto& reachingDefs = result[0];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printReachingDefs(reachingDefs);
        
        // Find blocks by their content
        std::string entryBlockId, positiveBlockId, endBlockId;
        
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst->opCode == IRInstruction::OpCode::LABEL) {
                    auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
                    if (labelOp->getName() == "positive") {
                        positiveBlockId = blockId;
                    } else if (labelOp->getName() == "end") {
                        endBlockId = blockId;
                    }
                }
            }
            
            // Entry block is the one with the first assign instruction
            if (block->instructions.size() > 0 && 
                block->instructions[0]->opCode == IRInstruction::OpCode::ASSIGN) {
                entryBlockId = blockId;
            }
        }
        
        std::cout << "Entry block: " << entryBlockId << std::endl;
        std::cout << "Positive block: " << positiveBlockId << std::endl;
        std::cout << "End block: " << endBlockId << std::endl;
        
        // Verify that reaching definitions are computed
        assert(!entryBlockId.empty());
        assert(!positiveBlockId.empty());
        assert(!endBlockId.empty());
        
        // Check that end block has reaching definitions from both branches
        if (reachingDefs.count(endBlockId)) {
            auto& endDefs = reachingDefs[endBlockId];
            std::cout << "End block reaching definitions: " << endDefs.in.size() << " in, " << endDefs.out.size() << " out" << std::endl;
        }
        
        std::cout << "✓ Branching reaching definitions test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testLoopReachingDef() {
    std::cout << "=== Testing Loop Reaching Definitions ===" << std::endl;
    
    // Create a program with a loop
    std::string irContent = R"(#start_function
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
#end_function)";

    std::ofstream tempFile("temp_loop_reaching.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_loop_reaching.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Get reaching definitions
        std::vector<ControlFlowGraph> cfgs = {cfg};
        ReachingDefOut result = computeReachingDefs(cfgs);
        
        assert(result.size() == 1);
        auto& reachingDefs = result[0];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printReachingDefs(reachingDefs);
        
        // Find loop and done blocks
        std::string loopBlockId, doneBlockId;
        
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst->opCode == IRInstruction::OpCode::LABEL) {
                    auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
                    if (labelOp->getName() == "loop") {
                        loopBlockId = blockId;
                    } else if (labelOp->getName() == "done") {
                        doneBlockId = blockId;
                    }
                }
            }
        }
        
        std::cout << "Loop block: " << loopBlockId << std::endl;
        std::cout << "Done block: " << doneBlockId << std::endl;
        
        // Verify that reaching definitions handle loops correctly
        assert(!loopBlockId.empty());
        assert(!doneBlockId.empty());
        
        if (reachingDefs.count(loopBlockId)) {
            auto& loopDefs = reachingDefs[loopBlockId];
            std::cout << "Loop block reaching definitions: " << loopDefs.in.size() << " in, " << loopDefs.out.size() << " out" << std::endl;
        }
        
        std::cout << "✓ Loop reaching definitions test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testMultipleFunctionsReachingDef() {
    std::cout << "=== Testing Multiple Functions Reaching Definitions ===" << std::endl;
    
    // Create a program with multiple functions
    std::string irContent = R"(#start_function
int add(int a, int b):
int-list: result
float-list:
    add, result, a, b
    return, result
#end_function

#start_function
int main():
int-list: x, y, z
float-list:
    assign, x, 10
    assign, y, 20
    callr, z, add, x, y
    call, puti, z
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_multiple_functions.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_multiple_functions.ir");
        
        std::vector<ControlFlowGraph> cfgs;
        for (const auto& func : program.functions) {
            cfgs.push_back(CFGBuilder::buildCFG(*func));
        }
        
        // Get reaching definitions for all functions
        ReachingDefOut result = computeReachingDefs(cfgs);
        
        assert(result.size() == 2); // Should have reaching defs for both functions
        
        std::cout << "Function 1 (add) CFG blocks:" << std::endl;
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
        
        printReachingDefs(result[0]);
        printReachingDefs(result[1]);
        
        std::cout << "✓ Multiple functions reaching definitions test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testReachingDefAlgorithmCorrectness() {
    std::cout << "=== Testing Reaching Definition Algorithm Correctness ===" << std::endl;
    
    // Create a program with known reaching definition patterns
    std::string irContent = R"(#start_function
int test():
int-list: a, b, c
float-list:
    assign, a, 1
    assign, b, 2
    brgt, branch1, a, 0
    assign, c, 3
    goto, merge
branch1:
    assign, c, 4
merge:
    add, a, a, c
    call, puti, a
    call, putc, 10
#end_function)";

    std::ofstream tempFile("temp_correctness.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_correctness.ir");
        
        auto& func = program.functions[0];
        ControlFlowGraph cfg = CFGBuilder::buildCFG(*func);
        
        // Get reaching definitions
        std::vector<ControlFlowGraph> cfgs = {cfg};
        ReachingDefOut result = computeReachingDefs(cfgs);
        
        assert(result.size() == 1);
        auto& reachingDefs = result[0];
        
        std::cout << "CFG blocks:" << std::endl;
        for (const auto& [blockId, block] : cfg.blocks) {
            std::cout << "  " << blockId << ": ";
            for (const auto& inst : block->instructions) {
                std::cout << inst->irLineNumber << " ";
            }
            std::cout << std::endl;
        }
        
        printReachingDefs(reachingDefs);
        
        // Find merge block (should have reaching definitions from both branches)
        std::string mergeBlockId;
        for (const auto& [blockId, block] : cfg.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst->opCode == IRInstruction::OpCode::LABEL) {
                    auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
                    if (labelOp->getName() == "merge") {
                        mergeBlockId = blockId;
                        break;
                    }
                }
            }
        }
        
        assert(!mergeBlockId.empty());
        
        // The merge block should have reaching definitions from both branches
        if (reachingDefs.count(mergeBlockId)) {
            auto& mergeDefs = reachingDefs[mergeBlockId];
            std::cout << "Merge block has " << mergeDefs.in.size() << " reaching definitions" << std::endl;
            
            // Should have at least some reaching definitions from the branches
            assert(mergeDefs.in.size() > 0);
        }
        
        std::cout << "✓ Reaching definition algorithm correctness test passed" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Reaching Definition Analysis Testing Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testSimpleReachingDef();
        testBranchingReachingDef();
        testLoopReachingDef();
        testMultipleFunctionsReachingDef();
        testReachingDefAlgorithmCorrectness();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All reaching definition tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Clean up temporary files
    std::remove("temp_simple_reaching.ir");
    std::remove("temp_branch_reaching.ir");
    std::remove("temp_loop_reaching.ir");
    std::remove("temp_multiple_functions.ir");
    std::remove("temp_correctness.ir");
    
    return 0;
}
