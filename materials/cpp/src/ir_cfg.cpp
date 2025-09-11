#include "ir.hpp"
#include <sstream>
#include <algorithm>

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
            // Save previous block if it exists and is not empty
            if (currentBlock && !currentBlock->instructions.empty()) {
                blocks.push_back(currentBlock);
            }
            
            // Create new block for this label, identified by line number
            auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
            std::string blockId = "L" + std::to_string(inst->irLineNumber);
            currentBlock = std::make_shared<BasicBlock>(blockId);
            
            // Add the label instruction to the new block
            currentBlock->instructions.push_back(inst);
            labelToBlockIndex[labelOp->getName()] = blocks.size();
        } else {
            // If no current block, create one identified by line number
            if (!currentBlock) {
                std::string blockId = "B" + std::to_string(inst->irLineNumber);
                currentBlock = std::make_shared<BasicBlock>(blockId);
            }
            
            // Add instruction to current block
            currentBlock->instructions.push_back(inst);
            
            // Check if this instruction ends the basic block
            // A basic block terminates on branch instructions or labels
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
    // Create a map from label names to block IDs for edge building
    std::unordered_map<std::string, std::string> labelToBlockId;
    
    // Add all blocks to CFG and build label mapping
    for (const auto& block : blocks) {
        cfg.addBlock(block);
        
        // Find label instructions in this block to map label names to block IDs
        for (const auto& inst : block->instructions) {
            if (inst->opCode == IRInstruction::OpCode::LABEL) {
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0]);
                labelToBlockId[labelOp->getName()] = block->id;
            }
        }
    }
    
    // Set entry block (first block)
    if (!blocks.empty()) {
        cfg.entryBlock = blocks[0]->id;
    }
    
    // Build edges by analyzing control flow instructions
    for (size_t i = 0; i < blocks.size(); ++i) {
        const auto& block = blocks[i];
        if (block->instructions.empty()) continue;
        
        const auto& lastInst = block->instructions.back();
        
        switch (lastInst->opCode) {
            case IRInstruction::OpCode::GOTO: {
                // GOTO only points to the target, no fall-through
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                if (labelToBlockId.count(labelOp->getName())) {
                    cfg.addEdge(block->id, labelToBlockId[labelOp->getName()]);
                }
                break;
            }
            case IRInstruction::OpCode::BREQ:
            case IRInstruction::OpCode::BRNEQ:
            case IRInstruction::OpCode::BRLT:
            case IRInstruction::OpCode::BRGT:
            case IRInstruction::OpCode::BRGEQ: {
                // Branch instructions point to both the target and the next block
                auto labelOp = std::dynamic_pointer_cast<IRLabelOperand>(lastInst->operands[0]);
                if (labelToBlockId.count(labelOp->getName())) {
                    cfg.addEdge(block->id, labelToBlockId[labelOp->getName()]);
                }
                
                // Add fall-through edge to next block
                if (i + 1 < blocks.size()) {
                    cfg.addEdge(block->id, blocks[i + 1]->id);
                }
                break;
            }
            case IRInstruction::OpCode::RETURN: {
                // RETURN doesn't point to any other block
                cfg.exitBlocks.push_back(block->id);
                break;
            }
            default: {
                // All other instructions fall through to the next block
                if (i + 1 < blocks.size()) {
                    cfg.addEdge(block->id, blocks[i + 1]->id);
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
    
    // Find and display entry block name
    std::string entryDisplayName = "unknown";
    if (cfg.blocks.count(cfg.entryBlock)) {
        entryDisplayName = getBlockDisplayName(cfg.blocks.at(cfg.entryBlock));
    }
    os << "Entry block: " << entryDisplayName << std::endl;
    
    // Find and display exit block names
    os << "Exit blocks: ";
    for (size_t i = 0; i < cfg.exitBlocks.size(); ++i) {
        if (i > 0) os << ", ";
        if (cfg.blocks.count(cfg.exitBlocks[i])) {
            os << getBlockDisplayName(cfg.blocks.at(cfg.exitBlocks[i]));
        } else {
            os << cfg.exitBlocks[i];
        }
    }
    os << std::endl << std::endl;
    
    // Print each basic block
    for (const auto& [blockId, block] : cfg.blocks) {
        std::string displayName = getBlockDisplayName(block);
        os << "Block: " << displayName << std::endl;
        
        os << "  Predecessors: ";
        for (size_t i = 0; i < block->predecessors.size(); ++i) {
            if (i > 0) os << ", ";
            if (cfg.blocks.count(block->predecessors[i])) {
                os << getBlockDisplayName(cfg.blocks.at(block->predecessors[i]));
            } else {
                os << block->predecessors[i];
            }
        }
        os << std::endl;
        
        os << "  Successors: ";
        for (size_t i = 0; i < block->successors.size(); ++i) {
            if (i > 0) os << ", ";
            if (cfg.blocks.count(block->successors[i])) {
                os << getBlockDisplayName(cfg.blocks.at(block->successors[i]));
            } else {
                os << block->successors[i];
            }
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
        std::string displayName = getBlockDisplayName(block);
        os << "  \"" << blockId << "\" [label=\"" << displayName;
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
