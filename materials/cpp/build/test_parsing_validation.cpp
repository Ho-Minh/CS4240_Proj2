#include "ir.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace ircpp;

void testInstructionParsing() {
    std::cout << "=== Testing Instruction Parsing ===" << std::endl;
    
    // Test each instruction type
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, c, arr[3]
float-list: x, y, z
    assign, a, 10
    assign, b, 5
    add, c, a, b
    sub, c, a, b
    mult, c, a, b
    div, c, a, b
    and, c, a, b
    or, c, a, b
    goto, label1
    breq, label1, a, b
    brneq, label1, a, b
    brlt, label1, a, b
    brgt, label1, a, b
    brgeq, label1, a, b
    return, c
    call, puti, a
    callr, b, geti
    array_store, a, arr, 0
    array_load, c, arr, 0
label1:
#end_function
)";

    std::ofstream tempFile("temp_instructions.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_instructions.ir");
        
        assert(program.functions.size() == 1);
        auto& func = program.functions[0];
        assert(func->name == "main");
        
        std::cout << "Parsed function '" << func->name << "' with " << func->instructions.size() << " instructions" << std::endl;
        
        // Verify specific instructions were parsed correctly
        int assignCount = 0, addCount = 0, gotoCount = 0, labelCount = 0;
        
        for (const auto& inst : func->instructions) {
            switch (inst->opCode) {
                case IRInstruction::OpCode::ASSIGN: assignCount++; break;
                case IRInstruction::OpCode::ADD: addCount++; break;
                case IRInstruction::OpCode::GOTO: gotoCount++; break;
                case IRInstruction::OpCode::LABEL: labelCount++; break;
                default: break;
            }
        }
        
        std::cout << "Instruction counts - ASSIGN: " << assignCount << ", ADD: " << addCount 
                  << ", GOTO: " << gotoCount << ", LABEL: " << labelCount << std::endl;
        
        assert(assignCount >= 2); // At least 2 assign instructions
        assert(addCount >= 1);    // At least 1 add instruction
        assert(gotoCount >= 1);   // At least 1 goto instruction
        assert(labelCount >= 1);  // At least 1 label
        
        std::cout << "✓ All instruction types parsed correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testTypeSystemIntegration() {
    std::cout << "=== Testing Type System Integration ===" << std::endl;
    
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, intArray[5]
float-list: x, y, floatArray[3]
    assign, a, 42
    assign, x, 3.14
    assign, b, a
    assign, y, x
    array_store, a, intArray, 0
    array_load, b, intArray, 0
    array_store, x, floatArray, 0
    array_load, y, floatArray, 0
    call, puti, b
    call, putf, y
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_types.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_types.ir");
        
        auto& func = program.functions[0];
        
        // Check that variables have correct types
        bool foundIntVar = false, foundFloatVar = false, foundIntArray = false, foundFloatArray = false;
        
        for (const auto& var : func->variables) {
            if (var->getName() == "a" && var->type.get() == IRIntType::get().get()) {
                foundIntVar = true;
            }
            if (var->getName() == "x" && var->type.get() == IRFloatType::get().get()) {
                foundFloatVar = true;
            }
            if (var->getName() == "intArray") {
                auto arrType = std::dynamic_pointer_cast<IRArrayType>(var->type);
                if (arrType && arrType->size == 5 && arrType->elementType.get() == IRIntType::get().get()) {
                    foundIntArray = true;
                }
            }
            if (var->getName() == "floatArray") {
                auto arrType = std::dynamic_pointer_cast<IRArrayType>(var->type);
                if (arrType && arrType->size == 3 && arrType->elementType.get() == IRFloatType::get().get()) {
                    foundFloatArray = true;
                }
            }
        }
        
        assert(foundIntVar);
        assert(foundFloatVar);
        assert(foundIntArray);
        assert(foundFloatArray);
        
        std::cout << "✓ Type system integration works correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testOperandValidation() {
    std::cout << "=== Testing Operand Validation ===" << std::endl;
    
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, c
float-list:
    assign, a, 10
    assign, b, 20
    add, c, a, b
    call, puti, c
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_operands.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile("temp_operands.ir");
        
        auto& func = program.functions[0];
        auto& inst = func->instructions[2]; // The ADD instruction
        
        assert(inst->opCode == IRInstruction::OpCode::ADD);
        assert(inst->operands.size() == 3);
        
        // Check operand types
        auto dest = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]);
        auto src1 = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[1]);
        auto src2 = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[2]);
        
        assert(dest != nullptr);
        assert(src1 != nullptr);
        assert(src2 != nullptr);
        
        assert(dest->getName() == "c");
        assert(src1->getName() == "a");
        assert(src2->getName() == "b");
        
        std::cout << "✓ Operand validation works correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testRoundTripParsing() {
    std::cout << "=== Testing Round-Trip Parsing ===" << std::endl;
    
    try {
        // Create a simple IR program without parameters for round-trip testing
        std::string simpleIR = R"(
#start_function
int main():
int-list: a, b, c
float-list: x, y
    assign, a, 10
    assign, b, 20
    add, c, a, b
    assign, x, 3.5
    assign, y, 2.5
    call, puti, c
    call, putf, x
#end_function
)";
        
        // Write simple IR to temp file
        std::ofstream tempFile1("temp_simple.ir");
        tempFile1 << simpleIR;
        tempFile1.close();
        
        // Parse original file
        IRReader reader;
        IRProgram original = reader.parseIRFile("temp_simple.ir");
        
        // Print to string
        std::stringstream printed;
        IRPrinter printer(printed);
        printer.printProgram(original);
        std::string printedContent = printed.str();
        
        // Write printed content to temp file
        std::ofstream tempFile2("temp_roundtrip.ir");
        tempFile2 << printedContent;
        tempFile2.close();
        
        // Parse the printed content
        IRProgram reparsed = reader.parseIRFile("temp_roundtrip.ir");
        
        // Compare function counts
        assert(original.functions.size() == reparsed.functions.size());
        
        // Compare function names and instruction counts
        for (size_t i = 0; i < original.functions.size(); ++i) {
            assert(original.functions[i]->name == reparsed.functions[i]->name);
            assert(original.functions[i]->instructions.size() == reparsed.functions[i]->instructions.size());
        }
        
        std::cout << "✓ Round-trip parsing works correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "IR Parsing Validation Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testInstructionParsing();
        testTypeSystemIntegration();
        testOperandValidation();
        testRoundTripParsing();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All parsing validation tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
