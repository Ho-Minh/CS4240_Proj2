# CS4240 Project 2: IR to MIPS32 Instruction Selector

This project implements an instruction selector that converts Intermediate Representation (IR) code to MIPS32 assembly language.

## Project Overview

The project consists of three main components:

1. **C++ IR to MIPS Converter** (`materials/cpp/`) - Converts IR code to MIPS32 assembly
2. **Java MIPS Interpreter** (`materials/mips-interpreter/`) - Executes MIPS assembly code
3. **Public Test Cases** (`materials/public_test_cases/`) - Test cases for validation

## Architecture

The instruction selector uses a **Strategy Pattern** design with these core components:

- **RegisterManager**: Manages register allocation and variable-to-register mapping
- **Instruction Selectors**: Specialized classes for different IR instruction types
  - `ArithmeticSelector`: Handles ADD, SUB, MULT, DIV, AND, OR
  - `BranchSelector`: Handles BREQ, BRNEQ, BRLT, BRGT, BRGEQ, GOTO, LABEL
  - `MemorySelector`: Handles ASSIGN, ARRAY_STORE, ARRAY_LOAD
  - `CallSelector`: Handles CALL, CALLR, RETURN
- **Selection Context**: Manages selection state and coordinates between selectors
- **MIPS Instruction System**: Defines MIPS32 instructions and operands

## Quick Start

### 1. Build the IR to MIPS Converter

```bash
cd materials/cpp
make
```

This creates:
- `bin/libircpp.a` - Static library with all components
- `bin/ir_to_mips` - Executable for IR to MIPS conversion

### 2. Build the MIPS Interpreter

```bash
cd materials/mips-interpreter
mkdir -p build
javac -d build -cp src src/main/java/mips/MIPSInterpreter.java
```

### 3. Convert and Test

```bash
# Convert IR to MIPS
cd materials/cpp
./bin/ir_to_mips ../public_test_cases/prime/prime.ir prime.s

# Run with MIPS interpreter
cd ../mips-interpreter
java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/prime/0.in ../cpp/prime.s

# Compare output
diff output ../public_test_cases/prime/0.out
```

## Testing Workflow

### Unit Testing Individual Components

Test individual components before full pipeline testing:

```bash
cd materials/cpp

# Run all unit tests
./bin/ir_to_mips --test-all

# Run specific component tests
./bin/ir_to_mips --test-register     # Test RegisterManager
./bin/ir_to_mips --test-arithmetic   # Test ArithmeticSelector
./bin/ir_to_mips --test-branch       # Test BranchSelector
./bin/ir_to_mips --test-memory       # Test MemorySelector
./bin/ir_to_mips --test-call         # Test CallSelector
./bin/ir_to_mips --test-selector     # Test IRToMIPSSelector
```

### Full Pipeline Testing

#### Single Test Case

```bash
cd materials/cpp

# Convert IR to MIPS
./bin/ir_to_mips ../public_test_cases/prime/prime.ir prime.s

# Run with MIPS interpreter
cd ../mips-interpreter
java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/prime/0.in ../cpp/prime.s > output

# Compare with expected output
diff output ../public_test_cases/prime/0.out
```

#### Complete Test Suite

```bash
#!/bin/bash
cd materials/cpp

# Build the converter
make

# Test all prime cases
echo "Testing prime cases..."
for i in {0..9}; do
    echo "Testing prime case $i"
    ./bin/ir_to_mips ../public_test_cases/prime/prime.ir prime_$i.s
    cd ../mips-interpreter
    java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/prime/$i.in ../cpp/prime_$i.s > output_$i
    if diff -q output_$i ../public_test_cases/prime/$i.out; then
        echo "✓ Prime case $i passed"
    else
        echo "✗ Prime case $i failed"
        echo "Expected:"
        cat ../public_test_cases/prime/$i.out
        echo "Got:"
        cat output_$i
    fi
    cd ../cpp
done

# Test all quicksort cases
echo "Testing quicksort cases..."
for i in {0..9}; do
    echo "Testing quicksort case $i"
    ./bin/ir_to_mips ../public_test_cases/quicksort/quicksort.ir quicksort_$i.s
    cd ../mips-interpreter
    java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/quicksort/$i.in ../cpp/quicksort_$i.s > output_$i
    if diff -q output_$i ../public_test_cases/quicksort/$i.out; then
        echo "✓ Quicksort case $i passed"
    else
        echo "✗ Quicksort case $i failed"
        echo "Expected:"
        cat ../public_test_cases/quicksort/$i.out
        echo "Got:"
        cat output_$i
    fi
    cd ../cpp
done
```

## Available Test Cases

### Prime Number Test (`materials/public_test_cases/prime/`)
- **IR file**: `prime.ir` - Tests prime number checking algorithm
- **Input files**: `0.in` through `9.in` - Various test inputs
- **Expected outputs**: `0.out` through `9.out` - Expected results

**Sample Input/Output:**
- Input (`0.in`): `1`
- Expected Output (`0.out`): `0` (1 is not prime)

### Quicksort Test (`materials/public_test_cases/quicksort/`)
- **IR file**: `quicksort.ir` - Tests quicksort sorting algorithm
- **Input files**: `0.in` through `9.in` - Various array inputs
- **Expected outputs**: `0.out` through `9.out` - Sorted arrays

## MIPS Interpreter Usage

### Basic Execution

```bash
# Run MIPS program
java -cp build main.java.mips.MIPSInterpreter file.s

# Run with input file
java -cp build main.java.mips.MIPSInterpreter --in input.txt file.s

# Run with input redirection
java -cp build main.java.mips.MIPSInterpreter file.s < input.txt
```

### Debug Mode

Use debug mode to step through your generated MIPS code:

```bash
java -cp build main.java.mips.MIPSInterpreter --debug file.s
```

Debug commands:
- `Enter` - Execute next instruction
- `p $register` - Print register value (e.g., `p $t0`)
- `x/n address` - Examine memory (e.g., `x/5 0($sp)`)
- `g label` - Go to label (e.g., `g main`)
- `exit` - Exit debugger

**Note**: When using debug mode with input files, use `--in` flag instead of input redirection:
```bash
java -cp build main.java.mips.MIPSInterpreter --debug --in input.txt file.s
```

## IR Instruction Reference

Your converter must handle these IR instruction types:

### Arithmetic Operations
- `add, dest, src1, src2` → `add $dest, $src1, $src2`
- `sub, dest, src1, src2` → `sub $dest, $src1, $src2`
- `mult, dest, src1, src2` → `mult $src1, $src2; mflo $dest`
- `div, dest, src1, src2` → `div $src1, $src2; mflo $dest`
- `and, dest, src1, src2` → `and $dest, $src1, $src2`
- `or, dest, src1, src2` → `or $dest, $src1, $src2`

### Branch Operations
- `breq, label, src1, src2` → `beq $src1, $src2, label`
- `brneq, label, src1, src2` → `bne $src1, $src2, label`
- `brlt, label, src1, src2` → `blt $src1, $src2, label`
- `brgt, label, src1, src2` → `bgt $src1, $src2, label`
- `brgeq, label, src1, src2` → `bge $src1, $src2, label`
- `goto, label` → `j label`
- `label:` → `label:` (no instruction)

### Memory Operations
- `assign, dest, src` → `move $dest, $src` or `li $dest, immediate`
- `array_load, dest, array, index` → `lw $dest, offset($array_base)`
- `array_store, value, array, index` → `sw $value, offset($array_base)`

### Function Operations
- `call, function, params...` → `jal function` with parameter setup
- `callr, dest, function, params...` → `jal function; move $dest, $v0`
- `return, value` → `move $v0, $value; jr $ra`

### System Calls
- `geti` → System call to read integer
- `puti, value` → System call to print integer
- `putc, value` → System call to print character

## Expected MIPS Output Format

Your converter should generate MIPS assembly following this structure:

```mips
.text
main:
    move $fp, $sp
    addi $sp, $sp, -stack_size
    
    # Function body instructions
    
    li $v0, 10
    syscall

function_name:
    move $fp, $sp
    addi $sp, $sp, -stack_size
    
    # Function body
    
    jr $ra
```

## MIPS32 Register Conventions

### Register Usage
- **Caller-saved**: `$t0-$t9`, `$a0-$a3`, `$v0-$v1`
- **Callee-saved**: `$s0-$s7`, `$fp`, `$ra`
- **Special**: `$sp` (stack pointer), `$zero` (always 0)

### Calling Convention
- **Parameters**: First 4 in `$a0-$a3`, rest on stack
- **Return value**: `$v0`
- **Return address**: `$ra`
- **Stack frame**: `$fp` points to frame base

## Troubleshooting

### Common Issues

1. **Build Issues**
   - Ensure you're in the `materials/cpp` directory
   - Check that all source files are present
   - Verify that the Makefile exists and is correct

2. **Test Failures**
   - Check that all selector classes are properly implemented
   - Verify that the RegisterManager is working correctly
   - Ensure that MIPS instruction generation is implemented

3. **Runtime Issues**
   - **"No instructions generated"**: The selector methods may not be implemented yet
   - **Segmentation fault**: Check for null pointer dereferences in selector implementations
   - **Compilation errors**: Ensure all required headers are included
   - **Wrong output**: Use debug mode to trace execution

### Debugging Tips

1. **Use Unit Tests**: Test individual components before full pipeline
2. **Use Debug Mode**: Step through generated MIPS code instruction by instruction
3. **Check Register Allocation**: Verify registers are allocated and used correctly
4. **Validate Assembly Syntax**: Ensure generated MIPS instructions are syntactically correct
5. **Trace Execution**: Use debug mode to see exactly what your code is doing

## File Structure

```
materials/
├── cpp/                           # C++ IR to MIPS converter
│   ├── include/                   # Header files
│   ├── src/                       # Source files
│   ├── Makefile                   # Build system
│   └── bin/                       # Built executables
├── mips-interpreter/              # Java MIPS interpreter
│   ├── src/                       # Java source files
│   └── build/                     # Compiled Java classes
├── public_test_cases/             # Test cases
│   ├── prime/                     # Prime number tests
│   └── quicksort/                 # Quicksort tests
└── example/                       # Example IR files
```

## Development Strategy

1. **Start with basic components**: Implement register manager and MIPS instruction formatting
2. **Implement simple selectors first**: Start with arithmetic operations
3. **Test incrementally**: Test each component before moving to the next
4. **Use debug mode**: Leverage MIPS interpreter debug features
5. **Validate with test cases**: Ensure output matches expected results

## Additional Resources

- **Unit Test Documentation**: `materials/cpp/README_UNIT_TESTS.md`
- **Instruction Selector Documentation**: `materials/cpp/README_INSTRUCTION_SELECTOR.md`
- **System Design Documentation**: `materials/cpp/System design documentation.pdf`

---

**Happy coding!** Start with unit tests, then move to full pipeline testing. The MIPS interpreter's debug mode will be invaluable for troubleshooting any issues with your generated assembly code.
