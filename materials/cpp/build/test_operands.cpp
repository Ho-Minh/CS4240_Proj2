#include "ir.hpp"
#include <iostream>
#include <cassert>

using namespace ircpp;

int main() {
    std::cout << "=== Testing Operand System ===" << std::endl;
    
    // Test constant operands
    auto intConst = std::make_shared<IRConstantOperand>(IRIntType::get(), "42", nullptr);
    assert(intConst->getValueString() == "42");
    std::cout << "✓ Int constant operand: " << intConst->toString() << std::endl;
    
    auto floatConst = std::make_shared<IRConstantOperand>(IRFloatType::get(), "3.14", nullptr);
    assert(floatConst->getValueString() == "3.14");
    std::cout << "✓ Float constant operand: " << floatConst->toString() << std::endl;
    
    // Test variable operands
    auto intVar = std::make_shared<IRVariableOperand>(IRIntType::get(), "x", nullptr);
    assert(intVar->getName() == "x");
    std::cout << "✓ Int variable operand: " << intVar->toString() << std::endl;
    
    auto floatVar = std::make_shared<IRVariableOperand>(IRFloatType::get(), "y", nullptr);
    assert(floatVar->getName() == "y");
    std::cout << "✓ Float variable operand: " << floatVar->toString() << std::endl;
    
    // Test function operands
    auto funcOp = std::make_shared<IRFunctionOperand>("main", nullptr);
    assert(funcOp->getName() == "main");
    std::cout << "✓ Function operand: " << funcOp->toString() << std::endl;
    
    // Test label operands
    auto labelOp = std::make_shared<IRLabelOperand>("loop", nullptr);
    assert(labelOp->getName() == "loop");
    std::cout << "✓ Label operand: " << labelOp->toString() << std::endl;
    
    std::cout << "\nAll operand tests passed!" << std::endl;
    return 0;
}
