#include "ir.hpp"
#include "reaching_def.hpp"
#include "dead_code.hpp"
#include <algorithm>
#include <optional>

// Helper function to get the variable defined by an instruction
std::optional<std::string> getDefinedVariable(const ircpp::IRInstruction &instr) {
    switch(instr.opCode) {
        case ircpp::IRInstruction::OpCode::ASSIGN:
        case ircpp::IRInstruction::OpCode::ADD:
        case ircpp::IRInstruction::OpCode::SUB:
        case ircpp::IRInstruction::OpCode::MULT:
        case ircpp::IRInstruction::OpCode::DIV:
        case ircpp::IRInstruction::OpCode::AND:
        case ircpp::IRInstruction::OpCode::OR:
        case ircpp::IRInstruction::OpCode::ARRAY_LOAD:
        case ircpp::IRInstruction::OpCode::CALLR:  // Return value assignments
            if (instr.operands.size() > 0) {
                auto varOp = std::dynamic_pointer_cast<ircpp::IRVariableOperand>(instr.operands[0]);
                if (varOp) {
                    return std::optional<std::string>(varOp->getName());
                }
            }
            return std::nullopt;
        default:
            return std::nullopt;
    }
}

// Helper function to get all variables used by an instruction
std::unordered_set<std::string> getUsedVariables(const ircpp::IRInstruction &instr) {
    std::unordered_set<std::string> usedVars;
    
    // Skip the first operand for assignment instructions (it's the destination)
    size_t startIndex = 0;
    if (instr.opCode == ircpp::IRInstruction::OpCode::ASSIGN ||
        instr.opCode == ircpp::IRInstruction::OpCode::ADD ||
        instr.opCode == ircpp::IRInstruction::OpCode::SUB ||
        instr.opCode == ircpp::IRInstruction::OpCode::MULT ||
        instr.opCode == ircpp::IRInstruction::OpCode::DIV ||
        instr.opCode == ircpp::IRInstruction::OpCode::AND ||
        instr.opCode == ircpp::IRInstruction::OpCode::OR ||
        instr.opCode == ircpp::IRInstruction::OpCode::ARRAY_LOAD ||
        instr.opCode == ircpp::IRInstruction::OpCode::ARRAY_STORE ||
        instr.opCode == ircpp::IRInstruction::OpCode::BREQ ||
        instr.opCode == ircpp::IRInstruction::OpCode::BRNEQ ||
        instr.opCode == ircpp::IRInstruction::OpCode::BRLT ||
        instr.opCode == ircpp::IRInstruction::OpCode::BRGT ||
        instr.opCode == ircpp::IRInstruction::OpCode::BRGEQ ||
        instr.opCode == ircpp::IRInstruction::OpCode::RETURN) {
        startIndex = 1;
    }
    
    for (size_t i = startIndex; i < instr.operands.size(); ++i) {
        auto varOp = std::dynamic_pointer_cast<ircpp::IRVariableOperand>(instr.operands[i]);
        if (varOp) {
            usedVars.insert(varOp->getName());
        }
    }
    
    return usedVars;
}

DeadCodeAnalysis findUnreachableInstructions(const ircpp::ControlFlowGraph &cfg) {
    DeadCodeAnalysis analysis;
    
    // Get reaching definitions to determine which instructions are reachable
    std::vector<ircpp::ControlFlowGraph> cfgs = {cfg};
    ReachingDefOut reachingDefs = computeReachingDefs(cfgs);
    auto& functionReachingDefs = reachingDefs[0];
    
    // Find all reachable instruction line numbers
    std::unordered_set<int> reachableInstructions;
    for (const auto& [blockName, blockDefs] : functionReachingDefs) {
        // Instructions that reach the exit of a block are reachable
        for (int lineNum : blockDefs.out) {
            reachableInstructions.insert(lineNum);
        }
    }
    
    // Find all instructions in the CFG
    std::unordered_set<int> allInstructions;
    for (const auto& [blockId, block] : cfg.blocks) {
        for (const auto& instr : block->instructions) {
            allInstructions.insert(instr->irLineNumber);
        }
    }
    
    // Unreachable instructions are those that exist but are not reachable
    for (int lineNum : allInstructions) {
        if (reachableInstructions.find(lineNum) == reachableInstructions.end()) {
            analysis.unreachableInstructions.insert(lineNum);
        }
    }
    
    return analysis;
}

DeadCodeAnalysis findUnusedAssignments(const ircpp::ControlFlowGraph &cfg, const std::unordered_map<std::string, BasicBlockReachingDef> &reachingDefs) {
    DeadCodeAnalysis analysis;
    
    // Track which variable definitions are used
    std::unordered_map<std::string, std::unordered_set<int>> variableDefs;
    std::unordered_map<std::string, std::unordered_set<int>> variableUses;
    
    // First pass: collect all variable definitions and uses
    for (const auto& [blockId, block] : cfg.blocks) {
        for (const auto& instr : block->instructions) {
            // Track definitions
            if (auto defVar = getDefinedVariable(*instr)) {
                variableDefs[*defVar].insert(instr->irLineNumber);
            }
            
            // Track uses
            auto usedVars = getUsedVariables(*instr);
            for (const auto& varName : usedVars) {
                variableUses[varName].insert(instr->irLineNumber);
            }
        }
    }
    
    // Second pass: find unused definitions
    for (const auto& [varName, defLines] : variableDefs) {
        // Skip if this variable is used anywhere
        if (variableUses.find(varName) != variableUses.end()) {
            continue;
        }
        
        // This variable is never used, so all its definitions are dead
        for (int lineNum : defLines) {
            analysis.unusedAssignments.insert(lineNum);
        }
    }
    
    return analysis;
}

bool isInstructionReachable(int lineNumber, const ircpp::ControlFlowGraph &cfg, const std::unordered_map<std::string, BasicBlockReachingDef> &reachingDefs) {
    // Check if this instruction line number appears in any reaching definition set
    for (const auto& [blockName, blockDefs] : reachingDefs) {
        if (blockDefs.out.find(lineNumber) != blockDefs.out.end()) {
            return true;
        }
    }
    return false;
}

bool isAssignmentUsed(int lineNumber, const ircpp::ControlFlowGraph &cfg, const std::unordered_map<std::string, BasicBlockReachingDef> &reachingDefs) {
    // Find the instruction
    const ircpp::IRInstruction* targetInstr = nullptr;
    for (const auto& [blockId, block] : cfg.blocks) {
        for (const auto& instr : block->instructions) {
            if (instr->irLineNumber == lineNumber) {
                targetInstr = instr.get();
                break;
            }
        }
        if (targetInstr) break;
    }
    
    if (!targetInstr) return false;
    
    // Get the variable defined by this instruction
    auto defVar = getDefinedVariable(*targetInstr);
    if (!defVar) return true; // Not an assignment, assume it's used
    
    // Check if this variable is used in any reaching definition
    for (const auto& [blockName, blockDefs] : reachingDefs) {
        for (int defLine : blockDefs.out) {
            if (defLine == lineNumber) continue; // Skip self
            
            // Find the instruction that uses this definition
            for (const auto& [blockId, block] : cfg.blocks) {
                for (const auto& instr : block->instructions) {
                    if (instr->irLineNumber == defLine) {
                        auto usedVars = getUsedVariables(*instr);
                        if (usedVars.find(*defVar) != usedVars.end()) {
                            return true; // This definition is used
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

std::unordered_set<int> getAllReachableInstructions(const ircpp::ControlFlowGraph &cfg) {
    std::unordered_set<int> reachable;
    
    // Get reaching definitions
    std::vector<ircpp::ControlFlowGraph> cfgs = {cfg};
    ReachingDefOut reachingDefs = computeReachingDefs(cfgs);
    auto& functionReachingDefs = reachingDefs[0];
    
    for (const auto& [blockName, blockDefs] : functionReachingDefs) {
        for (int lineNum : blockDefs.out) {
            reachable.insert(lineNum);
        }
    }
    
    return reachable;
}

DeadCodeResult analyzeDeadCode(const std::vector<ircpp::ControlFlowGraph> &functionCfgs) {
    DeadCodeResult result;
    
    // Get reaching definitions for all functions
    ReachingDefOut reachingDefs = computeReachingDefs(functionCfgs);
    
    for (size_t funcIdx = 0; funcIdx < functionCfgs.size(); ++funcIdx) {
        const auto& cfg = functionCfgs[funcIdx];
        const auto& funcReachingDefs = reachingDefs[funcIdx];
        
        DeadCodeAnalysis analysis;
        
        // Find unreachable instructions
        DeadCodeAnalysis unreachable = findUnreachableInstructions(cfg);
        analysis.unreachableInstructions = unreachable.unreachableInstructions;
        
        // Find unused assignments
        DeadCodeAnalysis unused = findUnusedAssignments(cfg, funcReachingDefs);
        analysis.unusedAssignments = unused.unusedAssignments;

        
        // Combine all dead instructions
        analysis.deadInstructions.insert(analysis.unreachableInstructions.begin(), analysis.unreachableInstructions.end());
        analysis.deadInstructions.insert(analysis.unusedAssignments.begin(), analysis.unusedAssignments.end());
        
        // Add to result
        result.functionResults.push_back({{"analysis", analysis}});
        
        // Update summary statistics
        result.totalDeadInstructions += analysis.deadInstructions.size();
        result.totalUnreachableInstructions += analysis.unreachableInstructions.size();
        result.totalUnusedAssignments += analysis.unusedAssignments.size();
    }
    
    return result;
}

DeadCodeResult eliminateDeadCode(const std::vector<ircpp::ControlFlowGraph> &functionCfgs) {
    // For now, just analyze - actual elimination would require modifying the CFG
    // which is more complex and may not be needed for analysis purposes
    return analyzeDeadCode(functionCfgs);
}
