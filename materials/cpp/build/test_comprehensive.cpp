#include "ir.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace ircpp;

void testArithmeticOperations() {
    std::cout << "=== Testing Arithmetic Operations ===" << std::endl;
    
    // Create a simple IR program with arithmetic operations
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, c, d, e
float-list: x, y, z
    assign, a, 10
    assign, b, 5
    add, c, a, b
    sub, d, a, b
    mult, e, a, b
    assign, x, 3.5
    assign, y, 2.0
    add, z, x, y
    call, puti, c
    call, putc, 32
    call, puti, d
    call, putc, 32
    call, puti, e
    call, putc, 32
    call, putf, z
    call, putc, 10
#end_function
)";

    // Write to temporary file
    std::ofstream tempFile("temp_arithmetic.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRInterpreter interpreter("temp_arithmetic.ir");
        
        // Capture output
        std::stringstream output;
        std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());
        
        interpreter.run();
        
        std::cout.rdbuf(oldCout);
        
        std::string result = output.str();
        std::cout << "Arithmetic test output: " << result << std::endl;
        
        // Expected: "15 5 50 5.5"
        assert(result.find("15") != std::string::npos);
        assert(result.find("5") != std::string::npos);
        assert(result.find("50") != std::string::npos);
        assert(result.find("5.5") != std::string::npos);
        
        std::cout << "✓ Arithmetic operations work correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testControlFlow() {
    std::cout << "=== Testing Control Flow ===" << std::endl;
    
    std::string irContent = R"(
#start_function
int main():
int-list: x, y, z
float-list:
    assign, x, 5
    assign, y, 3
    brgt, greater, x, y
    assign, z, 0
    goto, end
greater:
    assign, z, 1
end:
    call, puti, z
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_control.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRInterpreter interpreter("temp_control.ir");
        
        std::stringstream output;
        std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());
        
        interpreter.run();
        
        std::cout.rdbuf(oldCout);
        
        std::string result = output.str();
        std::cout << "Control flow test output: " << result << std::endl;
        
        // Should output "1" since 5 > 3
        assert(result.find("1") != std::string::npos);
        
        std::cout << "✓ Control flow works correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testArrayOperations() {
    std::cout << "=== Testing Array Operations ===" << std::endl;
    
    std::string irContent = R"(
#start_function
int main():
int-list: arr[5], i, val
float-list:
    assign, i, 0
    assign, val, 10
    array_store, val, arr, i
    assign, i, 1
    assign, val, 20
    array_store, val, arr, i
    assign, i, 0
    array_load, val, arr, i
    call, puti, val
    call, putc, 32
    assign, i, 1
    array_load, val, arr, i
    call, puti, val
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_array.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRInterpreter interpreter("temp_array.ir");
        
        std::stringstream output;
        std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());
        
        interpreter.run();
        
        std::cout.rdbuf(oldCout);
        
        std::string result = output.str();
        std::cout << "Array test output: " << result << std::endl;
        
        // Should output "10 20"
        assert(result.find("10") != std::string::npos);
        assert(result.find("20") != std::string::npos);
        
        std::cout << "✓ Array operations work correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testRealIRFile() {
    std::cout << "=== Testing Real IR File (example.ir) ===" << std::endl;
    
    try {
        // First test parsing
        IRReader reader;
        IRProgram program = reader.parseIRFile("../../example/example.ir");
        
        std::cout << "Parsed " << program.functions.size() << " functions:" << std::endl;
        for (const auto& func : program.functions) {
            std::cout << "  - " << func->name << " with " << func->instructions.size() << " instructions" << std::endl;
        }
        
        // Test printing
        std::stringstream printed;
        IRPrinter printer(printed);
        printer.printProgram(program);
        
        std::cout << "✓ Successfully parsed and printed real IR file" << std::endl;
        
        // Test execution with input "5"
        IRInterpreter interpreter("../../example/example.ir");
        
        std::stringstream output;
        std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());
        
        // Simulate input "5"
        std::stringstream input("5\n");
        std::streambuf* oldCin = std::cin.rdbuf(input.rdbuf());
        
        interpreter.run();
        
        std::cout.rdbuf(oldCout);
        std::cin.rdbuf(oldCin);
        
        std::string result = output.str();
        std::cout << "Fibonacci(5) output: " << result << std::endl;
        
        // Fibonacci(5) should be 5
        assert(result.find("5") != std::string::npos);
        
        std::cout << "✓ Real IR file execution works correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

void testComparisonOperators() {
    std::cout << "=== Testing Comparison Operators ===" << std::endl;
    
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, result
float-list:
    assign, a, 10
    assign, b, 5
    breq, eq_true, a, a
    assign, result, 0
    goto, eq_end
eq_true:
    assign, result, 1
eq_end:
    call, puti, result
    call, putc, 32
    
    brneq, neq_true, a, b
    assign, result, 0
    goto, neq_end
neq_true:
    assign, result, 1
neq_end:
    call, puti, result
    call, putc, 32
    
    brlt, lt_true, b, a
    assign, result, 0
    goto, lt_end
lt_true:
    assign, result, 1
lt_end:
    call, puti, result
    call, putc, 32
    
    brgt, gt_true, a, b
    assign, result, 0
    goto, gt_end
gt_true:
    assign, result, 1
gt_end:
    call, puti, result
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_comparison.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRInterpreter interpreter("temp_comparison.ir");
        
        std::stringstream output;
        std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());
        
        interpreter.run();
        
        std::cout.rdbuf(oldCout);
        
        std::string result = output.str();
        std::cout << "Comparison test output: " << result << std::endl;
        
        // Should output "1 1 1 1" (all comparisons true)
        assert(result.find("1 1 1 1") != std::string::npos);
        
        std::cout << "✓ Comparison operators work correctly" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Comprehensive IR System Testing" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testArithmeticOperations();
        testControlFlow();
        testArrayOperations();
        testComparisonOperators();
        testRealIRFile();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All comprehensive tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
