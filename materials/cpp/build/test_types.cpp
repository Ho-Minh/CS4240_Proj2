#include "ir.hpp"
#include <iostream>
#include <cassert>

using namespace ircpp;

int main() {
    std::cout << "=== Testing Type System ===" << std::endl;
    
    // Test singleton behavior
    auto int1 = IRIntType::get();
    auto int2 = IRIntType::get();
    assert(int1.get() == int2.get());
    std::cout << "✓ Int type singleton works" << std::endl;
    
    auto float1 = IRFloatType::get();
    auto float2 = IRFloatType::get();
    assert(float1.get() == float2.get());
    std::cout << "✓ Float type singleton works" << std::endl;
    
    // Test array type caching
    auto intArray1 = IRArrayType::get(int1, 10);
    auto intArray2 = IRArrayType::get(int1, 10);
    assert(intArray1.get() == intArray2.get());
    std::cout << "✓ Array type caching works" << std::endl;
    
    auto intArray3 = IRArrayType::get(int1, 20);
    assert(intArray1.get() != intArray3.get());
    std::cout << "✓ Different array sizes create different types" << std::endl;
    
    auto floatArray = IRArrayType::get(float1, 10);
    assert(intArray1.get() != floatArray.get());
    std::cout << "✓ Different element types create different array types" << std::endl;
    
    std::cout << "\nAll type system tests passed!" << std::endl;
    return 0;
}
