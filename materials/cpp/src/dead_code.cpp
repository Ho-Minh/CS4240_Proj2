#include "ir.hpp"
#include "reaching_def.hpp"
#include "dead_code.hpp"
#include <bits/stdc++.h>

DeadCodeResult analyzeDeadCode(const std::vector<ircpp::ControlFlowGraph> &functionCfgs) {
    DeadCodeResult result;
    
    // Get reaching definitions for all functions
    ReachingDefOut reachingDefs = computeReachingDefs(functionCfgs);

    std::map<int, std::string> blockNameMap;
    
    for (size_t funcIdx = 0; funcIdx < functionCfgs.size(); ++funcIdx) {

        const auto& cfg = functionCfgs[funcIdx];
        const auto& funcReachingDefs = reachingDefs[funcIdx];

        //Print funcReachingDefs
        std::cout << "Function " << funcIdx << " reaching defs: " << std::endl;
        for (const auto& [blockName, block] : cfg.blocks) {
            std::cout << "Block " << blockName << ": " << std::endl;
            //I need to print a few instructions in this block so I know which one it is
            for (const auto& instr : block->instructions) {
                std::cout << instr->irLineNumber << ", ";
            }
            std::cout << std::endl;
            for (const auto& def : funcReachingDefs.at(blockName).in) {
                std::cout << def << ", ";
            }
            std::cout << std::endl;
        }

         DeadCodeAnalysis analysis;

         std::unordered_set<int> marked;
         std::queue<ircpp::IRInstruction> workList;
         for (const auto& [blockName, block] : cfg.blocks) {
            for (const auto& instr : block->instructions) {
                blockNameMap[instr->irLineNumber] = blockName;
                if (isCritical(instr->opCode)) {
                    marked.insert(instr->irLineNumber);
                    if (instr->opCode != ircpp::IRInstruction::OpCode::LABEL) {
                        workList.push(*instr);
                    }
                }
            }
         }
         
         while (!workList.empty()) {
            ircpp::IRInstruction i = workList.front();
            workList.pop();
            //Case 1: If there's a def that reaches i in the same block, mark that def, continue
            std::string currentBlock = blockNameMap[i.irLineNumber];
            bool hasSameBlockDef = false;
            for (const auto& instr : cfg.blocks.at(currentBlock)->instructions) {
                bool definesUsedVar = false;
                if (instr->operands.size() > 0 && isDefiningInstruction(instr->opCode)) {
                    std::string definedVar = instr->operands[0]->value;
                    // Check if i uses this variable
                    for (const auto& operand : i.operands) {
                        if (operand->value == definedVar) {
                            definesUsedVar = true;
                            break;
                        }
                    }
                }
                if (definesUsedVar && instr->irLineNumber < i.irLineNumber) {
                    if (!marked.count(instr->irLineNumber)) {
                        marked.insert(instr->irLineNumber);
                        workList.push(*instr);
                    }
                    hasSameBlockDef = true;
                    break;
                }
            }
            if (hasSameBlockDef) {
                continue;
            }
            //Case 2: If there's a def that reaches i in another block, mark that def, continue
            for (const auto& [blockName, block] : cfg.blocks) {
                if (blockName == currentBlock) {
                    continue;
                }
                for (const auto& instr : block->instructions) {
                    // Check if instruction j defines variables that are used by instruction i
                    bool definesUsedVar = false;
                    // Check if instr defines a variable that i uses
                    // Only check instructions that actually define variables
                    if (instr->operands.size() > 0 && isDefiningInstruction(instr->opCode)) {
                        std::string definedVar = instr->operands[0]->value;
                        // Check if i uses this variable
                        for (const auto& operand : i.operands) {
                            if (operand->value == definedVar) {
                                definesUsedVar = true;
                                break;
                            }
                        }
                    }
                    
                    if (definesUsedVar) {
                        // Check if this definition reaches i (check reaching defs)
                        if (funcReachingDefs.at(currentBlock).in.count(instr->irLineNumber)) {
                            if (!marked.count(instr->irLineNumber)) {
                                marked.insert(instr->irLineNumber);
                                workList.push(*instr);
                            }
                        }
                    }
                }
            }
         }


         


         for (const auto& [blockName, block] : cfg.blocks) {
            for (const auto& instr : block->instructions) {
                if (marked.find(instr->irLineNumber) == marked.end()) {
                    analysis.deadInstructions.insert(instr->irLineNumber);
                }
            }
         }
        
        // Add to result
        result.functionResults.push_back({{"analysis", analysis}});
    }
    
    return result;
}

bool isDefiningInstruction(ircpp::IRInstruction::OpCode opCode) {

    return opCode == ircpp::IRInstruction::OpCode::ASSIGN ||
           opCode == ircpp::IRInstruction::OpCode::ADD ||
           opCode == ircpp::IRInstruction::OpCode::SUB ||
           opCode == ircpp::IRInstruction::OpCode::MULT ||
           opCode == ircpp::IRInstruction::OpCode::DIV ||
           opCode == ircpp::IRInstruction::OpCode::AND ||
           opCode == ircpp::IRInstruction::OpCode::OR ||
           opCode == ircpp::IRInstruction::OpCode::ARRAY_LOAD;
}

bool isCritical(ircpp::IRInstruction::OpCode opCode) {
    return opCode == ircpp::IRInstruction::OpCode::LABEL ||
           opCode == ircpp::IRInstruction::OpCode::GOTO ||
           opCode == ircpp::IRInstruction::OpCode::BREQ ||
           opCode == ircpp::IRInstruction::OpCode::BRNEQ ||
           opCode == ircpp::IRInstruction::OpCode::BRLT ||
           opCode == ircpp::IRInstruction::OpCode::BRGT ||
           opCode == ircpp::IRInstruction::OpCode::BRGEQ ||
           opCode == ircpp::IRInstruction::OpCode::RETURN ||
           opCode == ircpp::IRInstruction::OpCode::CALL ||
           opCode == ircpp::IRInstruction::OpCode::CALLR ||
           opCode == ircpp::IRInstruction::OpCode::ARRAY_STORE;
}
