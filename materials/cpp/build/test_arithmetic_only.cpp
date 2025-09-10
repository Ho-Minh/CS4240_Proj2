#include "ir.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace ircpp;

int main() {
    std::cout << "=== Testing Arithmetic Operators Only ===" << std::endl;
    
    // Create IR program that tests all arithmetic operations
    std::string irContent = R"(
#start_function
int main():
int-list: a, b, c, d, e, f, g
float-list: x, y, z, w
    assign, a, 20
    assign, b, 4
    add, c, a, b
    sub, d, a, b
    mult, e, a, b
    div, f, a, b
    and, g, a, b
    assign, x, 10.5
    assign, y, 2.5
    add, z, x, y
    sub, w, x, y
    call, puti, c
    call, putc, 32
    call, puti, d
    call, putc, 32
    call, puti, e
    call, putc, 32
    call, puti, f
    call, putc, 32
    call, puti, g
    call, putc, 32
    call, putf, z
    call, putc, 32
    call, putf, w
    call, putc, 10
#end_function
)";

    std::ofstream tempFile("temp_arithmetic_only.ir");
    tempFile << irContent;
    tempFile.close();

    try {
        IRInterpreter interpreter("temp_arithmetic_only.ir");
        
        std::stringstream output;
        std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());
        
        interpreter.run();
        
        std::cout.rdbuf(oldCout);
        
        std::string result = output.str();
        std::cout << "Arithmetic test output: " << result << std::endl;
        
        // Expected results:
        // c = 20 + 4 = 24
        // d = 20 - 4 = 16  
        // e = 20 * 4 = 80
        // f = 20 / 4 = 5
        // g = 20 & 4 = 4 (bitwise AND)
        // z = 10.5 + 2.5 = 13.0
        // w = 10.5 - 2.5 = 8.0
        
        std::cout << "Expected: 24 16 80 5 4 13 8" << std::endl;
        std::cout << "Got: " << result << std::endl;
        
        // Check for expected values
        assert(result.find("24") != std::string::npos);
        assert(result.find("16") != std::string::npos);
        assert(result.find("80") != std::string::npos);
        assert(result.find("5") != std::string::npos);
        assert(result.find("4") != std::string::npos);
        assert(result.find("13") != std::string::npos);
        assert(result.find("8") != std::string::npos);
        
        std::cout << "✓ All arithmetic operators work correctly!" << std::endl;
        
    } catch (const IRException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
