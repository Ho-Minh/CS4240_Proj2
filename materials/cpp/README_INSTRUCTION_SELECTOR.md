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

## Function Reference Guide

### Register Manager Functions (`register_manager.cpp`)

#### Core Allocation Functions
- **`RegisterManager()`** - Constructor that initializes available physical registers
  - **Purpose**: Sets up the register pool with MIPS physical registers ($t0-$t9, $s0-$s7, $a0-$a3, $v0-$v1, $sp, $fp, $ra)
  - **Used by**: All selectors when they need registers for variables

- **`allocateRegister(const std::string& varName)`** - Allocates a register for a variable
  - **Purpose**: Maps IR variables to physical/virtual registers
  - **Strategy**: Use physical registers first, fall back to virtual registers
  - **Used by**: All selectors when converting IR variables to MIPS operands
  - **Returns**: Shared pointer to allocated register

- **`getRegister(const std::string& varName)`** - Gets or allocates register for variable
  - **Purpose**: Lookup existing allocation or create new one
  - **Used by**: Selectors to get registers for IR variables
  - **Returns**: Shared pointer to register (existing or newly allocated)

- **`deallocateRegister(const std::string& varName)`** - Frees a register
  - **Purpose**: Returns register to available pool when variable goes out of scope
  - **Used by**: Function epilogue, end of basic blocks
  - **Effect**: Moves register back to availableRegs, removes from varToReg mapping

#### Immediate and Stack Management
- **`handleImmediate(int value)`** - Handles immediate values
  - **Purpose**: Creates temporary register for immediate values
  - **Used by**: Selectors when IR instruction has immediate operands
  - **Strategy**: Allocate temp register, use LI instruction
  - **Returns**: Shared pointer to register containing immediate

- **`allocateStackSpace(int size)`** - Allocates stack space
  - **Purpose**: Manages stack frame allocation for local variables
  - **Used by**: Function prologue, register spilling
  - **Returns**: Stack offset for allocated space

- **`spillRegister(std::shared_ptr<Register> reg)`** - Spills register to stack
  - **Purpose**: Saves register contents to stack when all physical registers are used
  - **Used by**: Register allocation when no physical registers available
  - **Returns**: Stack address where register was spilled

#### Register Classification
- **`getCallerSavedRegs()`** - Returns caller-saved registers
  - **Purpose**: Identifies registers that must be saved before function calls
  - **Used by**: Call selector for register preservation
  - **Returns**: Vector of caller-saved registers ($t0-$t9, $a0-$a3, $v0-$v1)

- **`getCalleeSavedRegs()`** - Returns callee-saved registers
  - **Purpose**: Identifies registers that must be preserved across function calls
  - **Used by**: Function prologue/epilogue
  - **Returns**: Vector of callee-saved registers ($s0-$s7, $fp, $ra)

#### Utility Functions
- **`getVirtualRegister()`** - Creates virtual register
  - **Purpose**: Generates virtual registers when physical registers are exhausted
  - **Used by**: Register allocation when spilling is needed
  - **Returns**: Virtual register with unique name ($vi0, $vi1, etc.)

- **`reset()`** - Resets register manager state
  - **Purpose**: Clears all allocations for new function
  - **Used by**: Function boundaries, test cleanup
  - **Effect**: Returns all registers to available pool, clears mappings

### Selection Context Functions (`instruction_selector.cpp`)

#### Label Management
- **`generateLabel(const std::string& irLabel)`** - Converts IR labels to MIPS labels
  - **Purpose**: Maps IR labels to unique MIPS label names
  - **Used by**: Branch and jump instructions
  - **Strategy**: Create unique MIPS label, store mapping
  - **Returns**: MIPS label name

#### Function Management
- **`generateFunctionPrologue(const IRFunction& func)`** - Generates function entry code
  - **Purpose**: Sets up function stack frame and saves registers
  - **Used by**: Function entry points
  - **Generates**: Stack allocation, register saves, frame pointer setup
  - **Returns**: Vector of prologue MIPS instructions

- **`generateFunctionEpilogue(const IRFunction& func)`** - Generates function exit code
  - **Purpose**: Restores registers and cleans up stack frame
  - **Used by**: Function exit points
  - **Generates**: Register restores, stack cleanup, return instruction
  - **Returns**: Vector of epilogue MIPS instructions

#### Operand Conversion
- **`convertOperand(std::shared_ptr<IROperand> irOp, SelectionContext& ctx)`** - Converts IR operands to MIPS
  - **Purpose**: Translates IR operands (variables, constants, labels) to MIPS operands
  - **Used by**: All selectors when processing instruction operands
  - **Handles**: Variables → registers, constants → immediates, labels → MIPS labels
  - **Returns**: MIPS operand (register, immediate, address, or label)

- **`getRegisterForOperand(std::shared_ptr<IROperand> irOp, SelectionContext& ctx)`** - Gets register for operand
  - **Purpose**: Ensures operand has a register allocated
  - **Used by**: Selectors that need registers for operands
  - **Strategy**: Allocate register for variables, create temp register for constants
  - **Returns**: Register containing the operand value

### Selector Registry Functions (`instruction_selector.cpp`)

#### Registry Management
- **`SelectorRegistry()`** - Constructor that registers all selectors
  - **Purpose**: Maps IR opcodes to their specialized selectors
  - **Used by**: Main selector for instruction dispatch
  - **Registers**: ADD→ArithmeticSelector, BREQ→BranchSelector, etc.

- **`registerSelector(IRInstruction::OpCode opcode, std::unique_ptr<InstructionSelector> selector)`** - Registers a selector
  - **Purpose**: Adds new opcode-to-selector mapping
  - **Used by**: Registry initialization, extensibility
  - **Effect**: Stores selector in selectors map

- **`select(const IRInstruction& ir, SelectionContext& ctx)`** - Dispatches to appropriate selector
  - **Purpose**: Routes IR instructions to correct selector
  - **Used by**: Main instruction selection process
  - **Strategy**: Look up selector by opcode, call its select method
  - **Returns**: MIPS instructions from the selector

### Main Selector Functions (`instruction_selector.cpp`)

#### Program-Level Selection
- **`IRToMIPSSelector()`** - Constructor
  - **Purpose**: Initializes the main selector with registry
  - **Used by**: Main program for IR-to-MIPS conversion

- **`selectProgram(const IRProgram& program)`** - Converts entire IR program
  - **Purpose**: Top-level function for program conversion
  - **Used by**: Main executable
  - **Process**: For each function, generate prologue, select instructions, generate epilogue
  - **Returns**: Complete MIPS instruction sequence

- **`selectFunction(const IRFunction& function)`** - Converts single function
  - **Purpose**: Converts one IR function to MIPS
  - **Used by**: selectProgram for each function
  - **Process**: Create context, generate prologue, select instructions, generate epilogue
  - **Returns**: MIPS instructions for the function

- **`selectInstruction(const IRInstruction& instruction, SelectionContext& ctx)`** - Converts single instruction
  - **Purpose**: Converts one IR instruction to MIPS
  - **Used by**: selectFunction for each instruction
  - **Process**: Use registry to find selector, call selector's select method
  - **Returns**: MIPS instructions for the IR instruction

#### Output Generation
- **`generateAssembly(const std::vector<MIPSInstruction>& instructions)`** - Generates assembly text
  - **Purpose**: Converts MIPS instructions to assembly text
  - **Used by**: File output, debugging
  - **Process**: Add section headers, format instructions, add comments
  - **Returns**: Complete assembly text

- **`writeAssemblyFile(const std::string& filename, const std::vector<MIPSInstruction>& instructions)`** - Writes assembly to file
  - **Purpose**: Outputs MIPS assembly to file
  - **Used by**: Main executable for file output
  - **Process**: Generate assembly text, write to file with proper formatting

### Arithmetic Selector Functions (`arithmetic_selector.cpp`)

#### Main Dispatch
- **`select(const IRInstruction& ir, SelectionContext& ctx)`** - Main arithmetic selector
  - **Purpose**: Routes arithmetic IR instructions to specific selectors
  - **Used by**: Registry for arithmetic opcodes (ADD, SUB, MULT, DIV, AND, OR)
  - **Process**: Switch on opcode, call appropriate specific selector
  - **Returns**: MIPS instructions for the arithmetic operation

#### Specific Arithmetic Operations
- **`selectAdd(const IRInstruction& ir, SelectionContext& ctx)`** - Handles ADD operations
  - **Purpose**: Converts IR ADD to MIPS add/addi
  - **Used by**: select() for ADD opcode
  - **Strategy**: Get registers for operands, use addi if second operand is immediate
  - **Returns**: MIPS add or addi instruction

- **`selectSub(const IRInstruction& ir, SelectionContext& ctx)`** - Handles SUB operations
  - **Purpose**: Converts IR SUB to MIPS sub
  - **Used by**: select() for SUB opcode
  - **Strategy**: Get registers for operands, generate sub instruction
  - **Returns**: MIPS sub instruction

- **`selectMult(const IRInstruction& ir, SelectionContext& ctx)`** - Handles MULT operations
  - **Purpose**: Converts IR MULT to MIPS mult + mflo
  - **Used by**: select() for MULT opcode
  - **Strategy**: MIPS multiplication uses HI/LO registers, need mflo to get result
  - **Returns**: MIPS mult and mflo instructions

- **`selectDiv(const IRInstruction& ir, SelectionContext& ctx)`** - Handles DIV operations
  - **Purpose**: Converts IR DIV to MIPS div + mflo
  - **Used by**: select() for DIV opcode
  - **Strategy**: MIPS division uses HI/LO registers, need mflo to get quotient
  - **Returns**: MIPS div and mflo instructions

- **`selectAnd(const IRInstruction& ir, SelectionContext& ctx)`** - Handles AND operations
  - **Purpose**: Converts IR AND to MIPS and/andi
  - **Used by**: select() for AND opcode
  - **Strategy**: Use andi if second operand is immediate
  - **Returns**: MIPS and or andi instruction

- **`selectOr(const IRInstruction& ir, SelectionContext& ctx)`** - Handles OR operations
  - **Purpose**: Converts IR OR to MIPS or/ori
  - **Used by**: select() for OR opcode
  - **Strategy**: Use ori if second operand is immediate
  - **Returns**: MIPS or or ori instruction

#### Optimization
- **`optimizeWithImmediate(MIPSOp opcode, std::shared_ptr<Register> dest, std::shared_ptr<MIPSOperand> src1, std::shared_ptr<MIPSOperand> src2, SelectionContext& ctx)`** - Optimizes immediate operations
  - **Purpose**: Uses immediate instructions when possible for better performance
  - **Used by**: All arithmetic selectors when second operand is immediate
  - **Strategy**: Check if src2 is immediate, use immediate instruction variant
  - **Returns**: Optimized MIPS instruction(s)

### Branch Selector Functions (`branch_selector.cpp`)

#### Main Dispatch
- **`select(const IRInstruction& ir, SelectionContext& ctx)`** - Main branch selector
  - **Purpose**: Routes branch/jump IR instructions to specific selectors
  - **Used by**: Registry for branch opcodes (BREQ, BRNEQ, BRLT, BRGT, BRGEQ, GOTO, LABEL)
  - **Process**: Switch on opcode, call appropriate specific selector
  - **Returns**: MIPS instructions for the branch/jump operation

#### Specific Branch Operations
- **`selectBranchEqual(const IRInstruction& ir, SelectionContext& ctx)`** - Handles BREQ operations
  - **Purpose**: Converts IR BREQ to MIPS beq
  - **Used by**: select() for BREQ opcode
  - **Strategy**: Get registers for operands, generate beq instruction with label
  - **Returns**: MIPS beq instruction

- **`selectBranchNotEqual(const IRInstruction& ir, SelectionContext& ctx)`** - Handles BRNEQ operations
  - **Purpose**: Converts IR BRNEQ to MIPS bne
  - **Used by**: select() for BRNEQ opcode
  - **Strategy**: Get registers for operands, generate bne instruction with label
  - **Returns**: MIPS bne instruction

- **`selectBranchLessThan(const IRInstruction& ir, SelectionContext& ctx)`** - Handles BRLT operations
  - **Purpose**: Converts IR BRLT to MIPS blt
  - **Used by**: select() for BRLT opcode
  - **Strategy**: Get registers for operands, generate blt instruction with label
  - **Returns**: MIPS blt instruction

- **`selectBranchGreaterThan(const IRInstruction& ir, SelectionContext& ctx)`** - Handles BRGT operations
  - **Purpose**: Converts IR BRGT to MIPS bgt
  - **Used by**: select() for BRGT opcode
  - **Strategy**: Get registers for operands, generate bgt instruction with label
  - **Returns**: MIPS bgt instruction

- **`selectBranchGreaterEqual(const IRInstruction& ir, SelectionContext& ctx)`** - Handles BRGEQ operations
  - **Purpose**: Converts IR BRGEQ to MIPS bge
  - **Used by**: select() for BRGEQ opcode
  - **Strategy**: Get registers for operands, generate bge instruction with label
  - **Returns**: MIPS bge instruction

- **`selectGoto(const IRInstruction& ir, SelectionContext& ctx)`** - Handles GOTO operations
  - **Purpose**: Converts IR GOTO to MIPS j
  - **Used by**: select() for GOTO opcode
  - **Strategy**: Convert IR label to MIPS label, generate j instruction
  - **Returns**: MIPS j instruction

- **`selectLabel(const IRInstruction& ir, SelectionContext& ctx)`** - Handles LABEL operations
  - **Purpose**: Converts IR LABEL to MIPS label
  - **Used by**: select() for LABEL opcode
  - **Strategy**: Convert IR label to MIPS label, no instruction generated
  - **Returns**: Empty vector (labels are handled separately)

#### Optimization
- **`optimizeBranchWithImmediate(MIPSOp opcode, std::shared_ptr<Register> src1, std::shared_ptr<MIPSOperand> src2, const std::string& targetLabel, SelectionContext& ctx)`** - Optimizes branches with immediates
  - **Purpose**: Uses immediate compare instructions when possible
  - **Used by**: Branch selectors when second operand is immediate
  - **Strategy**: Check if src2 is immediate, use immediate branch variant
  - **Returns**: Optimized MIPS branch instruction

### Memory Selector Functions (`memory_selector.cpp`)

#### Main Dispatch
- **`select(const IRInstruction& ir, SelectionContext& ctx)`** - Main memory selector
  - **Purpose**: Routes memory IR instructions to specific selectors
  - **Used by**: Registry for memory opcodes (ASSIGN, ARRAY_STORE, ARRAY_LOAD)
  - **Process**: Switch on opcode, call appropriate specific selector
  - **Returns**: MIPS instructions for the memory operation

#### Specific Memory Operations
- **`selectAssign(const IRInstruction& ir, SelectionContext& ctx)`** - Handles ASSIGN operations
  - **Purpose**: Converts IR ASSIGN to MIPS move/li
  - **Used by**: select() for ASSIGN opcode
  - **Strategy**: Check if source is immediate (use li) or variable (use move)
  - **Returns**: MIPS move or li instruction

- **`selectArrayStore(const IRInstruction& ir, SelectionContext& ctx)`** - Handles ARRAY_STORE operations
  - **Purpose**: Converts IR ARRAY_STORE to MIPS sw
  - **Used by**: select() for ARRAY_STORE opcode
  - **Strategy**: Calculate array address, generate sw instruction
  - **Returns**: MIPS sw instruction with calculated address

- **`selectArrayLoad(const IRInstruction& ir, SelectionContext& ctx)`** - Handles ARRAY_LOAD operations
  - **Purpose**: Converts IR ARRAY_LOAD to MIPS lw
  - **Used by**: select() for ARRAY_LOAD opcode
  - **Strategy**: Calculate array address, generate lw instruction
  - **Returns**: MIPS lw instruction with calculated address

#### Address Calculation
- **`calculateArrayAddress(std::shared_ptr<Register> resultReg, std::shared_ptr<Register> baseReg, std::shared_ptr<Register> indexReg, int elementSize, SelectionContext& ctx)`** - Calculates array element address
  - **Purpose**: Computes address of array element (base + index * element_size)
  - **Used by**: Array store/load operations
  - **Strategy**: Multiply index by element size, add to base address
  - **Returns**: MIPS instructions for address calculation

#### Assignment Optimization
- **`selectImmediateAssign(std::shared_ptr<Register> destReg, int immediateValue, SelectionContext& ctx)`** - Optimizes immediate assignments
  - **Purpose**: Uses li instruction for immediate value assignments
  - **Used by**: selectAssign when source is immediate
  - **Strategy**: Generate li instruction with immediate value
  - **Returns**: MIPS li instruction

- **`selectRegisterAssign(std::shared_ptr<Register> destReg, std::shared_ptr<Register> srcReg, SelectionContext& ctx)`** - Handles register assignments
  - **Purpose**: Uses move instruction for register-to-register assignments
  - **Used by**: selectAssign when source is variable
  - **Strategy**: Generate move instruction between registers
  - **Returns**: MIPS move instruction

### Call Selector Functions (`call_selector.cpp`)

#### Main Dispatch
- **`select(const IRInstruction& ir, SelectionContext& ctx)`** - Main call selector
  - **Purpose**: Routes call IR instructions to specific selectors
  - **Used by**: Registry for call opcodes (CALL, CALLR, RETURN)
  - **Process**: Switch on opcode, call appropriate specific selector
  - **Returns**: MIPS instructions for the call operation

#### Specific Call Operations
- **`selectCall(const IRInstruction& ir, SelectionContext& ctx)`** - Handles CALL operations
  - **Purpose**: Converts IR CALL to MIPS jal with parameter setup
  - **Used by**: select() for CALL opcode
  - **Strategy**: Setup parameters, save caller-saved registers, generate jal
  - **Returns**: MIPS instructions for function call

- **`selectCallWithReturn(const IRInstruction& ir, SelectionContext& ctx)`** - Handles CALLR operations
  - **Purpose**: Converts IR CALLR to MIPS jal + move for return value
  - **Used by**: select() for CALLR opcode
  - **Strategy**: Setup parameters, call function, move return value from $v0
  - **Returns**: MIPS instructions for function call with return value

- **`selectReturn(const IRInstruction& ir, SelectionContext& ctx)`** - Handles RETURN operations
  - **Purpose**: Converts IR RETURN to MIPS jr with return value setup
  - **Used by**: select() for RETURN opcode
  - **Strategy**: Move return value to $v0, restore registers, generate jr
  - **Returns**: MIPS instructions for function return

#### Parameter Management
- **`setupParameters(const std::vector<std::shared_ptr<IROperand>>& params, SelectionContext& ctx)`** - Sets up function parameters
  - **Purpose**: Places parameters in $a0-$a3 and on stack
  - **Used by**: Call selectors for parameter passing
  - **Strategy**: First 4 parameters in registers, rest on stack
  - **Returns**: MIPS instructions for parameter setup

- **`handleReturnValue(std::shared_ptr<Register> destReg, SelectionContext& ctx)`** - Handles return values
  - **Purpose**: Moves return value from $v0 to destination register
  - **Used by**: selectCallWithReturn for return value handling
  - **Strategy**: Generate move instruction from $v0
  - **Returns**: MIPS move instruction

#### Register Preservation
- **`saveCallerSavedRegisters(SelectionContext& ctx)`** - Saves caller-saved registers
  - **Purpose**: Saves registers that may be modified by called function
  - **Used by**: Call selectors before function calls
  - **Strategy**: Save $t0-$t9, $a0-$a3, $v0-$v1 to stack
  - **Returns**: MIPS instructions for register saves

- **`restoreCallerSavedRegisters(SelectionContext& ctx)`** - Restores caller-saved registers
  - **Purpose**: Restores registers after function call
  - **Used by**: Call selectors after function calls
  - **Strategy**: Restore saved registers from stack
  - **Returns**: MIPS instructions for register restores

#### Stack Management
- **`manageCallStack(int paramCount, SelectionContext& ctx)`** - Manages stack for function calls
  - **Purpose**: Allocates stack space for parameters and return addresses
  - **Used by**: Call selectors for stack management
  - **Strategy**: Allocate space for parameters beyond first 4
  - **Returns**: MIPS instructions for stack management

#### System Call Handling
- **`handleSystemCall(const std::string& functionName, const std::vector<std::shared_ptr<IROperand>>& params, SelectionContext& ctx)`** - Handles system calls
  - **Purpose**: Special handling for system calls (geti, puti, putc)
  - **Used by**: Call selectors for system functions
  - **Strategy**: Generate appropriate syscall with system call number
  - **Returns**: MIPS instructions for system call

### MIPS Instruction Functions (`mips_instructions.cpp`)

#### Instruction Formatting
- **`MIPSInstruction::toString()`** - Converts MIPS instruction to assembly text
  - **Purpose**: Formats MIPS instruction as assembly text
  - **Used by**: Assembly generation, debugging, file output
  - **Strategy**: Format as "[label:] opcode operand1, operand2, operand3"
  - **Returns**: Assembly text string

- **`opToString(MIPSOp op)`** - Converts MIPS opcode to string
  - **Purpose**: Maps MIPS opcode enum to assembly mnemonic
  - **Used by**: toString() for instruction formatting
  - **Returns**: Assembly mnemonic string

#### Register Helpers
- **`Registers::t0()` through `Registers::t9()`** - Creates temporary registers
  - **Purpose**: Helper functions for creating $t0-$t9 registers
  - **Used by**: Register allocation, test code
  - **Returns**: Shared pointer to register

- **`Registers::s0()` through `Registers::s7()`** - Creates saved registers
  - **Purpose**: Helper functions for creating $s0-$s7 registers
  - **Used by**: Register allocation, function prologue/epilogue
  - **Returns**: Shared pointer to register

- **`Registers::a0()` through `Registers::a3()`** - Creates argument registers
  - **Purpose**: Helper functions for creating $a0-$a3 registers
  - **Used by**: Parameter passing, function calls
  - **Returns**: Shared pointer to register

- **`Registers::v0()`, `Registers::v1()`** - Creates return value registers
  - **Purpose**: Helper functions for creating $v0-$v1 registers
  - **Used by**: Return value handling, function calls
  - **Returns**: Shared pointer to register

- **`Registers::sp()`, `Registers::fp()`, `Registers::ra()`** - Creates special registers
  - **Purpose**: Helper functions for creating special purpose registers
  - **Used by**: Stack management, function calls, return handling
  - **Returns**: Shared pointer to register

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

## Data Flow and Function Interactions

### High-Level Data Flow
1. **IR Program** → `IRToMIPSSelector::selectProgram()` → **MIPS Instructions**
2. **IR Function** → `IRToMIPSSelector::selectFunction()` → **MIPS Instructions**
3. **IR Instruction** → `SelectorRegistry::select()` → **Specialized Selector** → **MIPS Instructions**

### Function Call Hierarchy
```
main() (ir_to_mips.cpp)
├── IRToMIPSSelector::selectProgram()
│   ├── IRToMIPSSelector::selectFunction() [for each function]
│   │   ├── SelectionContext::generateFunctionPrologue()
│   │   ├── IRToMIPSSelector::selectInstruction() [for each instruction]
│   │   │   └── SelectorRegistry::select()
│   │   │       └── [Arithmetic|Branch|Memory|Call]Selector::select()
│   │   └── SelectionContext::generateFunctionEpilogue()
│   ├── IRToMIPSSelector::generateAssembly()
│   └── IRToMIPSSelector::writeAssemblyFile()
```

### Register Management Flow
```
RegisterManager::allocateRegister()
├── Check if variable already has register
├── If available physical register exists:
│   └── Allocate physical register
└── If no physical registers:
    ├── RegisterManager::getVirtualRegister()
    └── RegisterManager::spillRegister() [if needed]
```

### Instruction Selection Flow
```
IR Instruction → Selector → MIPS Instructions
├── ArithmeticSelector::select()
│   ├── Get registers for operands (RegisterManager::getRegister())
│   ├── Check for immediate optimization
│   └── Generate MIPS arithmetic instruction
├── BranchSelector::select()
│   ├── Get registers for operands
│   ├── Convert IR label to MIPS label (SelectionContext::generateLabel())
│   └── Generate MIPS branch instruction
├── MemorySelector::select()
│   ├── Handle assignment (immediate vs register)
│   ├── Calculate array addresses
│   └── Generate MIPS memory instruction
└── CallSelector::select()
    ├── Setup parameters (SelectionContext::setupParameters())
    ├── Save caller-saved registers
    ├── Generate jal instruction
    ├── Handle return value
    └── Restore caller-saved registers
```

### Key Function Dependencies

#### Register Manager Dependencies
- **Used by**: All selectors for register allocation
- **Depends on**: MIPS register definitions
- **Provides**: Register allocation, stack management, immediate handling

#### Selection Context Dependencies
- **Used by**: All selectors for state management
- **Depends on**: RegisterManager, label mapping
- **Provides**: Label generation, function prologue/epilogue, operand conversion

#### Selector Registry Dependencies
- **Used by**: Main selector for instruction dispatch
- **Depends on**: All specialized selectors
- **Provides**: Opcode-to-selector mapping, instruction routing

#### Specialized Selector Dependencies
- **Used by**: Registry for specific instruction types
- **Depends on**: RegisterManager, SelectionContext, MIPS instruction system
- **Provides**: IR-to-MIPS instruction conversion

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
