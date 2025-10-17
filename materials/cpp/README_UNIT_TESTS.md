# IR to MIPS Unit Tests

This document explains how to run unit tests for the individual components of the IR to MIPS instruction selector.

## Overview

The unit tests are integrated into the main `ir_to_mips.cpp` file and test each component individually:

- **RegisterManager**: Register allocation, deallocation, immediate handling, and stack management
- **ArithmeticSelector**: ADD, SUB, MULT, DIV, AND, OR operations
- **BranchSelector**: BREQ, BRNEQ, BRLT, BRGT, BRGEQ, GOTO, LABEL operations
- **MemorySelector**: ASSIGN, ARRAY_STORE, ARRAY_LOAD operations
- **CallSelector**: CALL, CALLR, RETURN operations
- **IRToMIPSSelector**: Main instruction selector and program conversion

## Quick Start

### On Linux/macOS:
```bash
cd materials/cpp
./test_runner.sh
```

### On Windows:
```cmd
cd materials\cpp
test_runner.bat
```

## Individual Test Commands

### Direct Command Line Usage

You can run individual tests directly using the compiled executable:

```bash
# Build the project first
make

# Run individual tests
./ir_to_mips --test-register     # Test RegisterManager
./ir_to_mips --test-arithmetic   # Test ArithmeticSelector
./ir_to_mips --test-branch       # Test BranchSelector
./ir_to_mips --test-memory       # Test MemorySelector
./ir_to_mips --test-call         # Test CallSelector
./ir_to_mips --test-selector     # Test IRToMIPSSelector
./ir_to_mips --test-all          # Run all tests
./ir_to_mips --test-help         # Show help
```

### Using Test Runner Scripts

#### Linux/macOS (test_runner.sh):
```bash
./test_runner.sh                    # Run all tests
./test_runner.sh register           # Test only RegisterManager
./test_runner.sh arithmetic         # Test only ArithmeticSelector
./test_runner.sh branch             # Test only BranchSelector
./test_runner.sh memory             # Test only MemorySelector
./test_runner.sh call               # Test only CallSelector
./test_runner.sh selector           # Test only IRToMIPSSelector
./test_runner.sh help               # Show help
```

#### Windows (test_runner.bat):
```cmd
test_runner.bat                     # Run all tests
test_runner.bat register            # Test only RegisterManager
test_runner.bat arithmetic          # Test only ArithmeticSelector
test_runner.bat branch              # Test only BranchSelector
test_runner.bat memory              # Test only MemorySelector
test_runner.bat call                # Test only CallSelector
test_runner.bat selector            # Test only IRToMIPSSelector
test_runner.bat help                # Show help
```

## Test Details

### RegisterManager Tests
- **Basic Register Allocation**: Tests allocating registers for variables
- **Register Lookup**: Tests retrieving previously allocated registers
- **Immediate Handling**: Tests handling immediate values
- **Stack Allocation**: Tests stack space allocation
- **Register Deallocation**: Tests freeing registers
- **Virtual Register Generation**: Tests virtual register creation
- **Reset Functionality**: Tests resetting the manager state

### ArithmeticSelector Tests
- **ADD Operation**: Tests addition with variables and immediates
- **SUB Operation**: Tests subtraction
- **MULT Operation**: Tests multiplication (with HI/LO registers)
- **DIV Operation**: Tests division (with HI/LO registers)
- **AND Operation**: Tests logical AND
- **OR Operation**: Tests logical OR
- **Immediate Optimization**: Tests using immediate instructions when possible

### BranchSelector Tests
- **BREQ**: Tests branch if equal
- **BRNEQ**: Tests branch if not equal
- **BRLT**: Tests branch if less than
- **BRGT**: Tests branch if greater than
- **BRGEQ**: Tests branch if greater or equal
- **GOTO**: Tests unconditional jump
- **LABEL**: Tests label definition
- **Immediate Branches**: Tests branches with immediate operands

### MemorySelector Tests
- **ASSIGN with Immediate**: Tests immediate value assignment
- **ASSIGN with Variable**: Tests variable-to-variable assignment
- **ARRAY_STORE**: Tests storing values to arrays
- **ARRAY_LOAD**: Tests loading values from arrays
- **Array with Immediate Index**: Tests array operations with constant indices

### CallSelector Tests
- **CALL**: Tests function calls without return values
- **CALLR**: Tests function calls with return values
- **RETURN**: Tests return statements
- **RETURN with Immediate**: Tests returning immediate values
- **Many Parameters**: Tests calls with multiple parameters (stack passing)
- **System Calls**: Tests special system calls (geti, puti, putc)

### IRToMIPSSelector Tests
- **Simple Program**: Tests converting a basic program with multiple instruction types
- **Assembly Generation**: Tests generating MIPS assembly text
- **Control Flow**: Tests programs with loops and branches

## Test Output

Each test provides detailed output showing:
- The IR instruction being tested
- The generated MIPS instructions
- Verification of instruction correctness
- Success/failure status

Example output:
```
=== ADD Results ===
  1: li $t0, 5
  2: add $t1, $t0, $t2
```

## Building and Running

### Prerequisites
- C++ compiler (g++, clang++, or MSVC)
- Make utility
- All source files in `src/` directory
- All header files in `include/` directory

### Build Process
1. Navigate to the `materials/cpp` directory
2. Run `make` to build the project
3. The executable `ir_to_mips` (or `ir_to_mips.exe` on Windows) will be created

### Running Tests
1. Use the test runner scripts for convenience, or
2. Run the executable directly with test flags

## Troubleshooting

### Build Issues
- Ensure you're in the `materials/cpp` directory
- Check that all source files are present
- Verify that the Makefile exists and is correct

### Test Failures
- Check that all selector classes are properly implemented
- Verify that the RegisterManager is working correctly
- Ensure that MIPS instruction generation is implemented

### Common Issues
- **"No instructions generated"**: The selector methods may not be implemented yet
- **Segmentation fault**: Check for null pointer dereferences in selector implementations
- **Compilation errors**: Ensure all required headers are included

## Extending Tests

To add new tests:

1. Add test cases to the appropriate test function in `ir_to_mips.cpp`
2. Use the `TestHelpers::createIRInstruction()` function to create test IR instructions
3. Use the `TestHelpers::printMIPSInstructions()` function to display results
4. Add verification logic using `TestHelpers::verifyOpcode()` and `TestHelpers::verifyOperandCount()`

## Integration with Development

These unit tests are designed to help with:
- **Development**: Test individual components as you implement them
- **Debugging**: Isolate issues to specific selectors
- **Verification**: Ensure each component works correctly
- **Regression Testing**: Catch issues when making changes

Run tests frequently during development to catch issues early!
