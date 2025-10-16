# IR to MIPS32 Instruction Selector

This project implements an instruction selector that converts Intermediate Representation (IR) code to MIPS32 assembly language.

## Architecture Overview

The instruction selector uses a **Strategy Pattern** design to provide clean, extensible, and maintainable code generation:

### Core Components

1. **MIPS Instruction System** (`mips_instructions.hpp/cpp`)
   - Defines MIPS32 instruction opcodes and operands
   - Handles register, immediate, address, and label operands
   - Provides helper functions for common MIPS registers

2. **Register Manager** (`register_manager.hpp/cpp`)
   - Manages register allocation and variable-to-register mapping
   - Handles register spilling to stack when needed
   - Tracks caller-saved and callee-saved registers

3. **Instruction Selectors** (Specialized classes for each IR instruction type)
   - `ArithmeticSelector`: Handles ADD, SUB, MULT, DIV, AND, OR
   - `BranchSelector`: Handles BREQ, BRNEQ, BRLT, BRGT, BRGEQ, GOTO, LABEL
   - `MemorySelector`: Handles ASSIGN, ARRAY_STORE, ARRAY_LOAD
   - `CallSelector`: Handles CALL, CALLR, RETURN

4. **Selection Context** (`instruction_selector.hpp/cpp`)
   - Manages selection state (register manager, labels, stack frames)
   - Handles function prologue/epilogue generation
   - Coordinates between different selectors

5. **Registry System**
   - Maps IR opcodes to their specialized selectors
   - Provides centralized dispatch for instruction selection

## File Structure

```
materials/cpp/
├── include/
│   ├── ir.hpp                    # Existing IR definitions
│   ├── mips_instructions.hpp     # MIPS instruction definitions
│   ├── register_manager.hpp      # Register allocation manager
│   ├── instruction_selector.hpp  # Base selector classes
│   ├── arithmetic_selector.hpp   # Arithmetic instruction selector
│   ├── branch_selector.hpp       # Branch instruction selector
│   ├── memory_selector.hpp       # Memory instruction selector
│   └── call_selector.hpp         # Function call selector
├── src/
│   ├── ir_core.cpp              # Existing IR implementation
│   ├── ir_types.cpp             # Existing IR types
│   ├── ir_reader.cpp            # Existing IR reader
│   ├── mips_instructions.cpp    # MIPS instruction implementation
│   ├── register_manager.cpp     # Register manager implementation
│   ├── instruction_selector.cpp # Base selector implementation
│   ├── arithmetic_selector.cpp  # Arithmetic selector implementation
│   ├── branch_selector.cpp      # Branch selector implementation
│   ├── memory_selector.cpp      # Memory selector implementation
│   ├── call_selector.cpp        # Call selector implementation
│   └── ir_to_mips.cpp          # Main executable
└── Makefile                     # Build system
```

## Implementation Tasks

Each file contains detailed TODO comments with specific implementation steps. Here's a summary of what needs to be implemented:

### 1. MIPS Instruction System (`mips_instructions.cpp`)
- **TODO**: Implement `MIPSInstruction::toString()` method
- Format MIPS instructions as: `[label:] opcode operand1, operand2, operand3`
- Handle special cases (no operands, single operand)

### 2. Register Manager (`register_manager.cpp`)
- **TODO**: Initialize available physical registers
- **TODO**: Implement `allocateRegister()` - allocate registers for variables
- **TODO**: Implement `deallocateRegister()` - free registers when variables go out of scope
- **TODO**: Implement `getRegister()` - lookup or allocate register for variable
- **TODO**: Implement `handleImmediate()` - handle immediate values
- **TODO**: Implement `spillRegister()` - spill registers to stack
- **TODO**: Implement `allocateStackSpace()` - manage stack allocation

### 3. Base Instruction Selector (`instruction_selector.cpp`)
- **TODO**: Implement `SelectionContext::generateLabel()` - create unique MIPS labels
- **TODO**: Implement function prologue/epilogue generation
- **TODO**: Implement operand conversion from IR to MIPS
- **TODO**: Implement selector registry registration
- **TODO**: Implement instruction selection dispatch
- **TODO**: Implement assembly text generation
- **TODO**: Implement file output

### 4. Arithmetic Selector (`arithmetic_selector.cpp`)
- **TODO**: Implement ADD selection → `add $dest, $src1, $src2`
- **TODO**: Implement SUB selection → `sub $dest, $src1, $src2`
- **TODO**: Implement MULT selection → `mult $src1, $src2; mflo $dest`
- **TODO**: Implement DIV selection → `div $src1, $src2; mflo $dest`
- **TODO**: Implement AND selection → `and $dest, $src1, $src2`
- **TODO**: Implement OR selection → `or $dest, $src1, $src2`
- **TODO**: Implement immediate optimization (use ADDI, ANDI, ORI when possible)

### 5. Branch Selector (`branch_selector.cpp`)
- **TODO**: Implement BREQ selection → `beq $src1, $src2, label`
- **TODO**: Implement BRNEQ selection → `bne $src1, $src2, label`
- **TODO**: Implement BRLT selection → `blt $src1, $src2, label`
- **TODO**: Implement BRGT selection → `bgt $src1, $src2, label`
- **TODO**: Implement BRGEQ selection → `bge $src1, $src2, label`
- **TODO**: Implement GOTO selection → `j label`
- **TODO**: Implement LABEL selection → `label:` (no instruction)

### 6. Memory Selector (`memory_selector.cpp`)
- **TODO**: Implement ASSIGN selection → `move $dest, $src` or `li $dest, immediate`
- **TODO**: Implement ARRAY_STORE selection → `sw $value, offset($array_base)`
- **TODO**: Implement ARRAY_LOAD selection → `lw $dest, offset($array_base)`
- **TODO**: Implement array address calculation
- **TODO**: Implement immediate vs register assignment optimization

### 7. Call Selector (`call_selector.cpp`)
- **TODO**: Implement CALL selection → `jal function` with parameter setup
- **TODO**: Implement CALLR selection → `jal function; move $dest, $v0`
- **TODO**: Implement RETURN selection → `move $v0, $value; jr $ra`
- **TODO**: Implement parameter passing (first 4 in $a0-$a3, rest on stack)
- **TODO**: Implement caller-saved register preservation
- **TODO**: Implement system call handling (geti, puti, putc)

## Build Instructions

```bash
cd materials/cpp
make clean
make
```

This will build:
- `libircpp.a` - Static library with all components
- `ir_to_mips` - Executable for IR to MIPS conversion

## Testing Instructions

### 1. Setup MIPS Interpreter

```bash
cd materials/mips-interpreter
mkdir -p build
javac -d build -cp src src/main/java/mips/MIPSInterpreter.java
```

### 2. Test Individual Components

Before testing the full pipeline, implement and test individual components:

#### Test Register Manager
```cpp
// Add test function to ir_to_mips.cpp
void testRegisterManager() {
    ircpp::RegisterManager rm;
    auto reg1 = rm.allocateRegister("x");
    auto reg2 = rm.allocateRegister("y");
    // Test register allocation, deallocation, spilling
}
```

#### Test Individual Selectors
```cpp
void testArithmeticSelector() {
    // Create test IR instruction: ADD x, y, z
    // Create selection context
    // Call selector
    // Verify MIPS output: add $t0, $t1, $t2
}
```

### 3. Test Full Pipeline

#### Basic Test
```bash
# Convert IR to MIPS
./bin/ir_to_mips ../public_test_cases/prime/prime.ir prime.s

# Run with MIPS interpreter
cd ../mips-interpreter
java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/prime/0.in ../cpp/prime.s

# Compare output
diff output ../public_test_cases/prime/0.out
```

#### Complete Test Suite
```bash
# Test all prime cases
for i in {0..9}; do
    ./bin/ir_to_mips ../public_test_cases/prime/prime.ir prime_$i.s
    cd ../mips-interpreter
    java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/prime/$i.in ../cpp/prime_$i.s > output_$i
    diff output_$i ../public_test_cases/prime/$i.out
    cd ../cpp
done

# Test quicksort cases
for i in {0..9}; do
    ./bin/ir_to_mips ../public_test_cases/quicksort/quicksort.ir quicksort_$i.s
    cd ../mips-interpreter
    java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/quicksort/$i.in ../cpp/quicksort_$i.s > output_$i
    diff output_$i ../public_test_cases/quicksort/$i.out
    cd ../cpp
done
```

### 4. Debugging

#### Use MIPS Interpreter Debug Mode
```bash
java -cp build main.java.mips.MIPSInterpreter --debug your_output.s
```

Debug commands:
- `Enter` - Execute next instruction
- `p $register` - Print register value
- `x/n address` - Examine memory
- `g label` - Go to label
- `exit` - Exit debugger

#### Common Issues and Solutions

1. **Register allocation errors**: Check register manager implementation
2. **Label not found**: Verify label mapping in SelectionContext
3. **Stack overflow**: Check stack management in function calls
4. **Wrong output**: Use debug mode to trace execution
5. **Assembly syntax errors**: Check MIPSInstruction::toString() implementation

## MIPS32 Instruction Reference

### Supported Instructions
- **Arithmetic**: `add`, `addi`, `sub`, `mul`, `div`, `and`, `andi`, `or`, `ori`, `sll`
- **Data Movement**: `li`, `lw`, `move`, `sw`, `la`
- **Branches**: `beq`, `bne`, `blt`, `bgt`, `bge`
- **Jumps**: `j`, `jal`, `jr`
- **System**: `syscall`

### Register Conventions
- **Caller-saved**: `$t0-$t9`, `$a0-$a3`, `$v0-$v1`
- **Callee-saved**: `$s0-$s7`, `$fp`, `$ra`
- **Special**: `$sp` (stack pointer), `$zero` (always 0)

### Calling Convention
- **Parameters**: First 4 in `$a0-$a3`, rest on stack
- **Return value**: `$v0`
- **Return address**: `$ra`
- **Stack frame**: `$fp` points to frame base

## Implementation Strategy

1. **Start with basic components**: Implement register manager and MIPS instruction formatting
2. **Implement simple selectors first**: Start with arithmetic operations
3. **Test incrementally**: Test each component before moving to the next
4. **Use debug mode**: Leverage MIPS interpreter debug features
5. **Validate with test cases**: Ensure output matches expected results

## Expected Output Format

The generated MIPS assembly should follow this structure:

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

This architecture provides a solid foundation for implementing a robust instruction selector that can be easily extended and maintained.
