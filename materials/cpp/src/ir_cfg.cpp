#include "ir.hpp"
#include <sstream>
#include <algorithm>

using namespace ircpp;

std::vector<std::shared_ptr<BasicBlock>> CFGBuilder::identifyBasicBlocks(const IRFunction& function) {
    std::vector<std::shared_ptr<BasicBlock>> blocks;
    std::unordered_map<std::string, int> labelToBlockIndex;
    
    // First pass: identify all labels and create basic blocks
    std::shared_ptr<BasicBlock> currentBlock = nullptr;
    int blockCounter = 0;
    
    for (size_t i = 0; i < function.instructions.size(); ++i) {
        const auto& inst = function.instructions[i];
        
        // Check if this is a label instruction
        if (inst->opCode == IRInstruction::OpCode::LABEL) {
            // Save previous block if it exists
            if (currentBlock && !currentBlock->instructions.empty()) {
                blocks.push_back(currentBlock);
            }
            
            // Create new block for this label
            auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
            std::string blockId = labelOp->getName();
            currentBlock = std::make_shared<BasicBlock>(blockId);
            labelToBlockIndex[blockId] = blocks.size();
        } else {
            // If no current block, create one
            if (!currentBlock) {
                std::string blockId = "entry_" + std::to_string(blockCounter++);
                currentBlock = std::make_shared<BasicBlock>(blockId);
            }
            
            // Add instruction to current block
            currentBlock->instructions.push_back(inst);
            
            // Check if this instruction ends the basic block
            bool endsBlock = false;
            switch (inst->opCode) {
                case IRInstruction::OpCode::GOTO:
                case IRInstruction::OpCode::RETURN:
                case IRInstruction::OpCode::BREQ:
                case IRInstruction::OpCode::BRNEQ:
                case IRInstruction::OpCode::BRLT:
                case IRInstruction::OpCode::BRGT:
                case IRInstruction::OpCode::BRGEQ:
                    endsBlock = true;
                    break;
                default:
                    break;
            }
            
            if (endsBlock) {
                blocks.push_back(currentBlock);
                currentBlock = nullptr;
            }
        }
    }
    
    // Add the last block if it exists
    if (currentBlock && !currentBlock->instructions.empty()) {
        blocks.push_back(currentBlock);
    }
    
    return blocks;
}

void CFGBuilder::buildEdges(ControlFlowGraph& cfg, const std::vector<std::shared_ptr<BasicBlock>>& blocks) {
    // Add all blocks to CFG
    for (const auto& block : blocks) {
        cfg.addBlock(block);
    }
    
    // Set entry block (first block)
    if (!blocks.empty()) {
        cfg.entryBlock = blocks[0]->id;
    }
    
    // Build edges by analyzing control flow instructions
    for (const auto& block : blocks) {
        if (block->instructions.empty()) continue;
        
        const auto& lastInst = block->instructions.back();
        
        switch (lastInst->opCode) {
            case IRInstruction::OpCode::GOTO: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                cfg.addEdge(block->id, labelOp->getName());
                break;
            }
            case IRInstruction::OpCode::BREQ:
            case IRInstruction::OpCode::BRNEQ:
            case IRInstruction::OpCode::BRLT:
            case IRInstruction::OpCode::BRGT:
            case IRInstruction::OpCode::BRGEQ: {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                cfg.addEdge(block->id, labelOp->getName());
                
                // Add fall-through edge to next block
                auto it = std::find_if(blocks.begin(), blocks.end(),
                    [&block](const std::shared_ptr<BasicBlock>& b) { return b->id == block->id; });
                if (it != blocks.end() && std::next(it) != blocks.end()) {
                    cfg.addEdge(block->id, (*std::next(it))->id);
                }
                break;
            }
            case IRInstruction::OpCode::RETURN: {
                cfg.exitBlocks.push_back(block->id);
                break;
            }
            default: {
                // Fall-through to next block
                auto it = std::find_if(blocks.begin(), blocks.end(),
                    [&block](const std::shared_ptr<BasicBlock>& b) { return b->id == block->id; });
                if (it != blocks.end() && std::next(it) != blocks.end()) {
                    cfg.addEdge(block->id, (*std::next(it))->id);
                } else {
                    // This might be an exit block
                    cfg.exitBlocks.push_back(block->id);
                }
                break;
            }
        }
    }
}

ControlFlowGraph CFGBuilder::buildCFG(const IRFunction& function) {
    ControlFlowGraph cfg;
    
    // Identify basic blocks
    auto blocks = identifyBasicBlocks(function);
    
    // Build edges between blocks
    buildEdges(cfg, blocks);
    
    return cfg;
}

void CFGBuilder::printCFG(const ControlFlowGraph& cfg, std::ostream& os) {
    os << "=== Control Flow Graph for function ===" << std::endl;
    os << "Entry block: " << cfg.entryBlock << std::endl;
    os << "Exit blocks: ";
    for (size_t i = 0; i < cfg.exitBlocks.size(); ++i) {
        if (i > 0) os << ", ";
        os << cfg.exitBlocks[i];
    }
    os << std::endl << std::endl;
    
    // Print each basic block
    for (const auto& [blockId, block] : cfg.blocks) {
        os << "Block: " << blockId << std::endl;
        os << "  Predecessors: ";
        for (size_t i = 0; i < block->predecessors.size(); ++i) {
            if (i > 0) os << ", ";
            os << block->predecessors[i];
        }
        os << std::endl;
        
        os << "  Successors: ";
        for (size_t i = 0; i < block->successors.size(); ++i) {
            if (i > 0) os << ", ";
            os << block->successors[i];
        }
        os << std::endl;
        
        os << "  Instructions:" << std::endl;
        for (const auto& inst : block->instructions) {
            os << "    ";
            switch (inst->opCode) {
                case IRInstruction::OpCode::ASSIGN: os << "assign"; break;
                case IRInstruction::OpCode::ADD: os << "add"; break;
                case IRInstruction::OpCode::SUB: os << "sub"; break;
                case IRInstruction::OpCode::MULT: os << "mult"; break;
                case IRInstruction::OpCode::DIV: os << "div"; break;
                case IRInstruction::OpCode::AND: os << "and"; break;
                case IRInstruction::OpCode::OR: os << "or"; break;
                case IRInstruction::OpCode::GOTO: os << "goto"; break;
                case IRInstruction::OpCode::BREQ: os << "breq"; break;
                case IRInstruction::OpCode::BRNEQ: os << "brneq"; break;
                case IRInstruction::OpCode::BRLT: os << "brlt"; break;
                case IRInstruction::OpCode::BRGT: os << "brgt"; break;
                case IRInstruction::OpCode::BRGEQ: os << "brgeq"; break;
                case IRInstruction::OpCode::RETURN: os << "return"; break;
                case IRInstruction::OpCode::CALL: os << "call"; break;
                case IRInstruction::OpCode::CALLR: os << "callr"; break;
                case IRInstruction::OpCode::ARRAY_STORE: os << "array_store"; break;
                case IRInstruction::OpCode::ARRAY_LOAD: os << "array_load"; break;
                case IRInstruction::OpCode::LABEL: os << "label"; break;
            }
            
            for (const auto& op : inst->operands) {
                os << ", " << op->toString();
            }
            os << std::endl;
        }
        os << std::endl;
    }
}

void CFGBuilder::printCFGDot(const ControlFlowGraph& cfg, std::ostream& os) {
    os << "digraph CFG {" << std::endl;
    os << "  rankdir=TB;" << std::endl;
    os << "  node [shape=box, style=filled, fillcolor=lightblue];" << std::endl;
    
    // Add nodes
    for (const auto& [blockId, block] : cfg.blocks) {
        os << "  \"" << blockId << "\" [label=\"" << blockId;
        if (!block->instructions.empty()) {
            os << "\\n";
            for (const auto& inst : block->instructions) {
                switch (inst->opCode) {
                    case IRInstruction::OpCode::ASSIGN: os << "assign"; break;
                    case IRInstruction::OpCode::ADD: os << "add"; break;
                    case IRInstruction::OpCode::SUB: os << "sub"; break;
                    case IRInstruction::OpCode::MULT: os << "mult"; break;
                    case IRInstruction::OpCode::DIV: os << "div"; break;
                    case IRInstruction::OpCode::AND: os << "and"; break;
                    case IRInstruction::OpCode::OR: os << "or"; break;
                    case IRInstruction::OpCode::GOTO: os << "goto"; break;
                    case IRInstruction::OpCode::BREQ: os << "breq"; break;
                    case IRInstruction::OpCode::BRNEQ: os << "brneq"; break;
                    case IRInstruction::OpCode::BRLT: os << "brlt"; break;
                    case IRInstruction::OpCode::BRGT: os << "brgt"; break;
                    case IRInstruction::OpCode::BRGEQ: os << "brgeq"; break;
                    case IRInstruction::OpCode::RETURN: os << "return"; break;
                    case IRInstruction::OpCode::CALL: os << "call"; break;
                    case IRInstruction::OpCode::CALLR: os << "callr"; break;
                    case IRInstruction::OpCode::ARRAY_STORE: os << "array_store"; break;
                    case IRInstruction::OpCode::ARRAY_LOAD: os << "array_load"; break;
                    case IRInstruction::OpCode::LABEL: os << "label"; break;
                }
                os << "\\n";
            }
        }
        os << "\"];" << std::endl;
    }
    
    // Add edges
    for (const auto& [blockId, block] : cfg.blocks) {
        for (const auto& successor : block->successors) {
            os << "  \"" << blockId << "\" -> \"" << successor << "\";" << std::endl;
        }
    }
    
    os << "}" << std::endl;
}
