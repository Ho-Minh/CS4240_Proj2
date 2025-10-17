#include <bits/stdc++.h>
#include "ir.hpp"
#include "instruction_selector.hpp"
#include "arithmetic_selector.hpp"
#include "branch_selector.hpp"
#include "memory_selector.hpp"
#include "call_selector.hpp"
#include "mips_instructions.hpp"
#include "register_manager.hpp"

// ============================================================================
// UNIT TEST FUNCTIONS FOR INDIVIDUAL COMPONENTS
// ============================================================================

// Helper functions for creating test data
namespace TestHelpers {
    // Create a test IR instruction
    std::shared_ptr<ircpp::IRInstruction> createIRInstruction(
        ircpp::IRInstruction::OpCode opcode,
        const std::vector<std::string>& operandValues,
        int lineNumber = 1) {
        
        std::vector<std::shared_ptr<ircpp::IROperand>> operands;
        for (const auto& value : operandValues) {
            // Determine operand type based on value format
            if (value[0] == '%') {
                // Variable operand
                auto varOp = std::make_shared<ircpp::IRVariableOperand>(
                    ircpp::IRIntType::get(), std::string(value), nullptr);
                operands.push_back(varOp);
            } else if (value[0] == '@') {
                // Function operand
                auto funcOp = std::make_shared<ircpp::IRFunctionOperand>(std::string(value), nullptr);
                operands.push_back(funcOp);
            } else if (value[0] == 'L') {
                // Label operand
                auto labelOp = std::make_shared<ircpp::IRLabelOperand>(std::string(value), nullptr);
                operands.push_back(labelOp);
            } else {
                // Constant operand
                auto constOp = std::make_shared<ircpp::IRConstantOperand>(
                    ircpp::IRIntType::get(), std::string(value), nullptr);
                operands.push_back(constOp);
            }
        }
        
        return std::make_shared<ircpp::IRInstruction>(opcode, operands, lineNumber);
    }
    
    // Create a selection context for testing
    std::unique_ptr<ircpp::SelectionContext> createTestContext() {
        auto regManager = std::make_unique<ircpp::RegisterManager>();
        return std::make_unique<ircpp::SelectionContext>(*regManager);
    }
    
    // Print MIPS instructions for verification
    void printMIPSInstructions(const std::vector<ircpp::MIPSInstruction>& instructions, 
                              const std::string& testName) {
        std::cout << "\n=== " << testName << " Results ===" << std::endl;
        for (size_t i = 0; i < instructions.size(); ++i) {
            std::cout << "  " << (i + 1) << ": " << instructions[i].toString() << std::endl;
        }
        if (instructions.empty()) {
            std::cout << "  No instructions generated" << std::endl;
        }
        std::cout << std::endl;
    }
    
    // Verify that instruction contains expected opcode
    bool verifyOpcode(const ircpp::MIPSInstruction& inst, ircpp::MIPSOp expectedOp) {
        return inst.op == expectedOp;
    }
    
    // Verify that instruction has expected number of operands
    bool verifyOperandCount(const ircpp::MIPSInstruction& inst, size_t expectedCount) {
        return inst.operands.size() == expectedCount;
    }
}

// ============================================================================
// REGISTER MANAGER TESTS
// ============================================================================

using namespace ircpp;

#define CHECK(cond) do { if(!(cond)){ std::cerr<<"FAIL: "<<#cond<<" @ "<<__LINE__<<"\n"; std::abort(); } } while(0)

static bool unique_names(const std::vector<std::shared_ptr<Register>>& regs){
    std::unordered_set<std::string> s;
    for (auto& r: regs) if(!s.insert(r->name).second) return false;
    return true;
}

void testRegisterManager() {
    std::cout << "[RM] starting comprehensive tests...\n";
    RegisterManager rm;

    // 1) Mapping reuse
    auto r1a = rm.allocateRegister("x");
    auto r1b = rm.getRegister("x");
    CHECK(r1a == r1b);

    // 2) Distinct mapping for different vars
    auto r2 = rm.allocateRegister("y");
    CHECK(r1a->name != r2->name);

    // 3) Pool exhaustion → virtuals appear
    // We don't rely on a hardcoded pool size; we just push until one is virtual.
    std::vector<std::shared_ptr<Register>> many;
    for (int i = 0; i < 64; ++i) many.push_back(rm.allocateRegister("v"+std::to_string(i)));
    bool sawVirtual = false;
    for (auto& r: many) if (!r->isPhysical) { sawVirtual = true; break; }
    CHECK(sawVirtual);

    // 4) Deallocation returns physical to pool (observable via reuse)
    // Free "y", allocate a fresh var and expect it can reuse some physical
    rm.deallocateRegister("y");
    auto r3 = rm.allocateRegister("z");
    CHECK(r3->isPhysical); // should come from pool (at least one returned)

    // 5) Virtual register generator: unique names, marked virtual
    auto v0 = rm.getVirtualRegister();
    auto v1 = rm.getVirtualRegister();
    CHECK(!v0->isPhysical && !v1->isPhysical && v0->name != v1->name);

    // 6) Stack allocator: 8-byte alignment, negative offsets, monotonic growth
    int o1 = rm.allocateStackSpace(4);    // -> -8
    int o2 = rm.allocateStackSpace(8);    // -> -16
    int o3 = rm.allocateStackSpace(1);    // -> -24 (rounded to 8)
    CHECK(o1 < 0 && o2 < 0 && o3 < 0);
    CHECK((o1 % 8) == 0 && (o2 % 8) == 0 && (o3 % 8) == 0);
    CHECK(o1 > o2 && o2 > o3);            // more negative as we allocate more

    // 7) Spill location shape: fp-relative negative offset, 4-byte slot (policy)
    auto someReg = r1a; // any reg
    auto addr = rm.spillRegister(someReg);
    CHECK(addr && addr->base && addr->base->name == Registers::fp()->name);
    CHECK(addr->offset < 0 && (addr->offset % 8) == 0);

    // 8) Caller/callee sets: non-empty, unique, disjointness sanity
    auto caller = rm.getCallerSavedRegs();
    auto callee = rm.getCalleeSavedRegs();
    CHECK(!caller.empty() && !callee.empty());
    CHECK(unique_names(caller) && unique_names(callee));
    // Basic disjointness check (allows $ra/$fp in callee only)
    std::unordered_set<std::string> cset;
    for (auto& r: caller) cset.insert(r->name);
    bool overlap_ok = true;
    for (auto& r: callee) {
        if (cset.count(r->name)) {
            // If overlap exists, it must be intentional; normally we expect none.
            overlap_ok = false; break;
        }
    }
    CHECK(overlap_ok);

    // 9) Reset restores fresh state
    rm.reset();
    // After reset, new allocations should be physical again
    auto r_after = rm.allocateRegister("a");
    CHECK(r_after->isPhysical);
    CHECK(rm.getStackOffset() == 0);

    std::cout << "[RM] all tests passed.\n";
}

// ============================================================================
// ARITHMETIC SELECTOR TESTS
// ============================================================================

void testArithmeticSelector() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TESTING ARITHMETIC SELECTOR" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    ircpp::ArithmeticSelector selector;
    auto context = TestHelpers::createTestContext();
    
    // Test 1: ADD operation
    std::cout << "\n--- Test 1: ADD Operation ---" << std::endl;
    auto addInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ADD, 
        {"%result", "%a", "%b"});
    auto addMIPS = selector.select(*addInst, *context);
    TestHelpers::printMIPSInstructions(addMIPS, "ADD");
    
    // Test 2: SUB operation
    std::cout << "\n--- Test 2: SUB Operation ---" << std::endl;
    auto subInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::SUB, 
        {"%result", "%a", "%b"});
    auto subMIPS = selector.select(*subInst, *context);
    TestHelpers::printMIPSInstructions(subMIPS, "SUB");
    
    // Test 3: MULT operation
    std::cout << "\n--- Test 3: MULT Operation ---" << std::endl;
    auto multInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::MULT, 
        {"%result", "%a", "%b"});
    auto multMIPS = selector.select(*multInst, *context);
    TestHelpers::printMIPSInstructions(multMIPS, "MULT");
    
    // Test 4: DIV operation
    std::cout << "\n--- Test 4: DIV Operation ---" << std::endl;
    auto divInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::DIV, 
        {"%result", "%a", "%b"});
    auto divMIPS = selector.select(*divInst, *context);
    TestHelpers::printMIPSInstructions(divMIPS, "DIV");
    
    // Test 5: AND operation
    std::cout << "\n--- Test 5: AND Operation ---" << std::endl;
    auto andInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::AND, 
        {"%result", "%a", "%b"});
    auto andMIPS = selector.select(*andInst, *context);
    TestHelpers::printMIPSInstructions(andMIPS, "AND");
    
    // Test 6: OR operation
    std::cout << "\n--- Test 6: OR Operation ---" << std::endl;
    auto orInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::OR, 
        {"%result", "%a", "%b"});
    auto orMIPS = selector.select(*orInst, *context);
    TestHelpers::printMIPSInstructions(orMIPS, "OR");
    
    // Test 7: ADD with immediate (optimization test)
    std::cout << "\n--- Test 7: ADD with Immediate ---" << std::endl;
    auto addImmInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ADD, 
        {"%result", "%a", "5"});
    auto addImmMIPS = selector.select(*addImmInst, *context);
    TestHelpers::printMIPSInstructions(addImmMIPS, "ADD with Immediate");
}

// ============================================================================
// BRANCH SELECTOR TESTS
// ============================================================================

void testBranchSelector() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TESTING BRANCH SELECTOR" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    ircpp::BranchSelector selector;
    auto context = TestHelpers::createTestContext();
    
    // Test 1: BREQ (Branch Equal)
    std::cout << "\n--- Test 1: BREQ Operation ---" << std::endl;
    auto breqInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::BREQ, 
        {"%a", "%b", "L1"});
    auto breqMIPS = selector.select(*breqInst, *context);
    TestHelpers::printMIPSInstructions(breqMIPS, "BREQ");
    
    // Test 2: BRNEQ (Branch Not Equal)
    std::cout << "\n--- Test 2: BRNEQ Operation ---" << std::endl;
    auto brneqInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::BRNEQ, 
        {"%a", "%b", "L2"});
    auto brneqMIPS = selector.select(*brneqInst, *context);
    TestHelpers::printMIPSInstructions(brneqMIPS, "BRNEQ");
    
    // Test 3: BRLT (Branch Less Than)
    std::cout << "\n--- Test 3: BRLT Operation ---" << std::endl;
    auto brltInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::BRLT, 
        {"%a", "%b", "L3"});
    auto brltMIPS = selector.select(*brltInst, *context);
    TestHelpers::printMIPSInstructions(brltMIPS, "BRLT");
    
    // Test 4: BRGT (Branch Greater Than)
    std::cout << "\n--- Test 4: BRGT Operation ---" << std::endl;
    auto brgtInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::BRGT, 
        {"%a", "%b", "L4"});
    auto brgtMIPS = selector.select(*brgtInst, *context);
    TestHelpers::printMIPSInstructions(brgtMIPS, "BRGT");
    
    // Test 5: BRGEQ (Branch Greater or Equal)
    std::cout << "\n--- Test 5: BRGEQ Operation ---" << std::endl;
    auto brgeqInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::BRGEQ, 
        {"%a", "%b", "L5"});
    auto brgeqMIPS = selector.select(*brgeqInst, *context);
    TestHelpers::printMIPSInstructions(brgeqMIPS, "BRGEQ");
    
    // Test 6: GOTO (Unconditional Jump)
    std::cout << "\n--- Test 6: GOTO Operation ---" << std::endl;
    auto gotoInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::GOTO, 
        {"L6"});
    auto gotoMIPS = selector.select(*gotoInst, *context);
    TestHelpers::printMIPSInstructions(gotoMIPS, "GOTO");
    
    // Test 7: LABEL (Label Definition)
    std::cout << "\n--- Test 7: LABEL Operation ---" << std::endl;
    auto labelInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::LABEL, 
        {"L7"});
    auto labelMIPS = selector.select(*labelInst, *context);
    TestHelpers::printMIPSInstructions(labelMIPS, "LABEL");
    
    // Test 8: Branch with immediate
    std::cout << "\n--- Test 8: Branch with Immediate ---" << std::endl;
    auto brImmInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::BREQ, 
        {"%a", "10", "L8"});
    auto brImmMIPS = selector.select(*brImmInst, *context);
    TestHelpers::printMIPSInstructions(brImmMIPS, "BREQ with Immediate");
}

// ============================================================================
// MEMORY SELECTOR TESTS
// ============================================================================

void testMemorySelector() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TESTING MEMORY SELECTOR" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    ircpp::MemorySelector selector;
    auto context = TestHelpers::createTestContext();
    
    // Test 1: ASSIGN with immediate
    std::cout << "\n--- Test 1: ASSIGN with Immediate ---" << std::endl;
    auto assignImmInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ASSIGN, 
        {"%result", "42"});
    auto assignImmMIPS = selector.select(*assignImmInst, *context);
    TestHelpers::printMIPSInstructions(assignImmMIPS, "ASSIGN with Immediate");
    
    // Test 2: ASSIGN with variable
    std::cout << "\n--- Test 2: ASSIGN with Variable ---" << std::endl;
    auto assignVarInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ASSIGN, 
        {"%result", "%source"});
    auto assignVarMIPS = selector.select(*assignVarInst, *context);
    TestHelpers::printMIPSInstructions(assignVarMIPS, "ASSIGN with Variable");
    
    // Test 3: ARRAY_STORE
    std::cout << "\n--- Test 3: ARRAY_STORE Operation ---" << std::endl;
    auto arrayStoreInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ARRAY_STORE, 
        {"%array", "%index", "%value"});
    auto arrayStoreMIPS = selector.select(*arrayStoreInst, *context);
    TestHelpers::printMIPSInstructions(arrayStoreMIPS, "ARRAY_STORE");
    
    // Test 4: ARRAY_LOAD
    std::cout << "\n--- Test 4: ARRAY_LOAD Operation ---" << std::endl;
    auto arrayLoadInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ARRAY_LOAD, 
        {"%result", "%array", "%index"});
    auto arrayLoadMIPS = selector.select(*arrayLoadInst, *context);
    TestHelpers::printMIPSInstructions(arrayLoadMIPS, "ARRAY_LOAD");
    
    // Test 5: ARRAY_STORE with immediate index
    std::cout << "\n--- Test 5: ARRAY_STORE with Immediate Index ---" << std::endl;
    auto arrayStoreImmInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ARRAY_STORE, 
        {"%array", "5", "%value"});
    auto arrayStoreImmMIPS = selector.select(*arrayStoreImmInst, *context);
    TestHelpers::printMIPSInstructions(arrayStoreImmMIPS, "ARRAY_STORE with Immediate Index");
    
    // Test 6: ARRAY_LOAD with immediate index
    std::cout << "\n--- Test 6: ARRAY_LOAD with Immediate Index ---" << std::endl;
    auto arrayLoadImmInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::ARRAY_LOAD, 
        {"%result", "%array", "3"});
    auto arrayLoadImmMIPS = selector.select(*arrayLoadImmInst, *context);
    TestHelpers::printMIPSInstructions(arrayLoadImmMIPS, "ARRAY_LOAD with Immediate Index");
}

// ============================================================================
// CALL SELECTOR TESTS
// ============================================================================

void testCallSelector() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TESTING CALL SELECTOR" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    ircpp::CallSelector selector;
    auto context = TestHelpers::createTestContext();
    
    // Test 1: CALL (no return value)
    std::cout << "\n--- Test 1: CALL Operation ---" << std::endl;
    auto callInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::CALL, 
        {"@function", "%arg1", "%arg2"});
    auto callMIPS = selector.select(*callInst, *context);
    TestHelpers::printMIPSInstructions(callMIPS, "CALL");
    
    // Test 2: CALLR (with return value)
    std::cout << "\n--- Test 2: CALLR Operation ---" << std::endl;
    auto callrInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::CALLR, 
        {"%result", "@function", "%arg1"});
    auto callrMIPS = selector.select(*callrInst, *context);
    TestHelpers::printMIPSInstructions(callrMIPS, "CALLR");
    
    // Test 3: RETURN
    std::cout << "\n--- Test 3: RETURN Operation ---" << std::endl;
    auto returnInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::RETURN, 
        {"%value"});
    auto returnMIPS = selector.select(*returnInst, *context);
    TestHelpers::printMIPSInstructions(returnMIPS, "RETURN");
    
    // Test 4: RETURN with immediate
    std::cout << "\n--- Test 4: RETURN with Immediate ---" << std::endl;
    auto returnImmInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::RETURN, 
        {"0"});
    auto returnImmMIPS = selector.select(*returnImmInst, *context);
    TestHelpers::printMIPSInstructions(returnImmMIPS, "RETURN with Immediate");
    
    // Test 5: CALL with many parameters (to test stack parameter passing)
    std::cout << "\n--- Test 5: CALL with Many Parameters ---" << std::endl;
    auto callManyInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::CALL, 
        {"@function", "%arg1", "%arg2", "%arg3", "%arg4", "%arg5"});
    auto callManyMIPS = selector.select(*callManyInst, *context);
    TestHelpers::printMIPSInstructions(callManyMIPS, "CALL with Many Parameters");
    
    // Test 6: System call (geti)
    std::cout << "\n--- Test 6: System Call (geti) ---" << std::endl;
    auto syscallInst = TestHelpers::createIRInstruction(
        ircpp::IRInstruction::OpCode::CALLR, 
        {"%result", "@geti"});
    auto syscallMIPS = selector.select(*syscallInst, *context);
    TestHelpers::printMIPSInstructions(syscallMIPS, "System Call (geti)");
}

// ============================================================================
// MAIN INSTRUCTION SELECTOR TESTS
// ============================================================================

void testInstructionSelector() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TESTING MAIN INSTRUCTION SELECTOR" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    ircpp::IRToMIPSSelector selector;
    
    // Test 1: Simple program with multiple instruction types
    std::cout << "\n--- Test 1: Simple Program Conversion ---" << std::endl;
    
    // Create a simple test program
    ircpp::IRProgram testProgram;
    auto testFunction = std::make_shared<ircpp::IRFunction>(
        "test_func",
        ircpp::IRIntType::get(),
        std::vector<std::shared_ptr<ircpp::IRVariableOperand>>(), // no parameters
        std::vector<std::shared_ptr<ircpp::IRVariableOperand>>(), // no local variables
        std::vector<std::shared_ptr<ircpp::IRInstruction>>()
    );
    
    // Add some test instructions
    testFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::ASSIGN, {"%x", "10"}));
    testFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::ASSIGN, {"%y", "20"}));
    testFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::ADD, {"%z", "%x", "%y"}));
    testFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::RETURN, {"%z"}));
    
    testProgram.functions.push_back(testFunction);
    
    try {
        auto mipsInstructions = selector.selectProgram(testProgram);
        TestHelpers::printMIPSInstructions(mipsInstructions, "Simple Program");
        
        // Test assembly generation
        std::cout << "\n--- Test 2: Assembly Generation ---" << std::endl;
        std::string assembly = selector.generateAssembly(mipsInstructions);
        std::cout << "Generated Assembly:" << std::endl;
        std::cout << assembly << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in program conversion: " << e.what() << std::endl;
    }
    
    // Test 3: Function with control flow
    std::cout << "\n--- Test 3: Function with Control Flow ---" << std::endl;
    auto controlFlowFunction = std::make_shared<ircpp::IRFunction>(
        "control_flow_func",
        ircpp::IRIntType::get(),
        std::vector<std::shared_ptr<ircpp::IRVariableOperand>>(),
        std::vector<std::shared_ptr<ircpp::IRVariableOperand>>(),
        std::vector<std::shared_ptr<ircpp::IRInstruction>>()
    );
    
    // Add control flow instructions
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::ASSIGN, {"%i", "0"}));
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::LABEL, {"L1"}));
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::BREQ, {"%i", "10", "L2"}));
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::ADD, {"%i", "%i", "1"}));
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::GOTO, {"L1"}));
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::LABEL, {"L2"}));
    controlFlowFunction->instructions.push_back(
        TestHelpers::createIRInstruction(ircpp::IRInstruction::OpCode::RETURN, {"%i"}));
    
    ircpp::IRProgram controlFlowProgram;
    controlFlowProgram.functions.push_back(controlFlowFunction);
    
    try {
        auto controlFlowMIPS = selector.selectProgram(controlFlowProgram);
        TestHelpers::printMIPSInstructions(controlFlowMIPS, "Control Flow Program");
    } catch (const std::exception& e) {
        std::cout << "Error in control flow conversion: " << e.what() << std::endl;
    }
}

// ============================================================================
// TEST RUNNER FUNCTIONS
// ============================================================================

void runAllTests() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "RUNNING ALL UNIT TESTS" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    testRegisterManager();
    testArithmeticSelector();
    testBranchSelector();
    testMemorySelector();
    testCallSelector();
    testInstructionSelector();
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "ALL TESTS COMPLETED" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

void printTestUsage() {
    std::cout << "\nUnit Test Usage:" << std::endl;
    std::cout << "  " << "ir_to_mips --test-register     # Test RegisterManager" << std::endl;
    std::cout << "  " << "ir_to_mips --test-arithmetic   # Test ArithmeticSelector" << std::endl;
    std::cout << "  " << "ir_to_mips --test-branch       # Test BranchSelector" << std::endl;
    std::cout << "  " << "ir_to_mips --test-memory       # Test MemorySelector" << std::endl;
    std::cout << "  " << "ir_to_mips --test-call         # Test CallSelector" << std::endl;
    std::cout << "  " << "ir_to_mips --test-selector     # Test IRToMIPSSelector" << std::endl;
    std::cout << "  " << "ir_to_mips --test-all          # Run all tests" << std::endl;
    std::cout << "  " << "ir_to_mips --test-help         # Show this help" << std::endl;
    std::cout << "\nNormal usage:" << std::endl;
    std::cout << "  " << "ir_to_mips <input.ir> <output.s>" << std::endl;
}

int main(int argc, char* argv[]) {
    // Handle test command line arguments
    if (argc >= 2) {
        std::string firstArg(argv[1]);
        
        if (firstArg == "--test-register") {
            testRegisterManager();
            return 0;
        } else if (firstArg == "--test-arithmetic") {
            testArithmeticSelector();
            return 0;
        } else if (firstArg == "--test-branch") {
            testBranchSelector();
            return 0;
        } else if (firstArg == "--test-memory") {
            testMemorySelector();
            return 0;
        } else if (firstArg == "--test-call") {
            testCallSelector();
            return 0;
        } else if (firstArg == "--test-selector") {
            testInstructionSelector();
            return 0;
        } else if (firstArg == "--test-all") {
            runAllTests();
            return 0;
        } else if (firstArg == "--test-help") {
            printTestUsage();
            return 0;
        }
    }
    
    // Normal usage: ./ir_to_mips input.ir output.s
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.ir> <output.s>" << std::endl;
        std::cerr << "       " << argv[0] << " --test-help" << std::endl;
        return 1;
    }
    
    std::string inputFile(argv[1]);
    std::string outputFile(argv[2]);
    
    try {
        // TODO: Implement IR to MIPS conversion
        // Steps:
        // 1. Parse IR file using IRReader
        // 2. Create instruction selector
        // 3. Convert IR program to MIPS instructions
        // 4. Generate assembly text
        // 5. Write to output file
        
        // Parse IR file
        ircpp::IRReader reader;
        ircpp::IRProgram program = reader.parseIRFile(inputFile);
        
        // Create instruction selector
        ircpp::IRToMIPSSelector selector;
        
        // Convert to MIPS
        std::vector<ircpp::MIPSInstruction> mipsInstructions = selector.selectProgram(program);
        
        // Generate and write assembly
        std::string assembly = selector.generateAssembly(mipsInstructions);
        selector.writeAssemblyFile(outputFile, mipsInstructions);
        
        std::cout << "Successfully converted " << inputFile << " to " << outputFile << std::endl;
        
    } catch (const ircpp::IRException& e) {
        // TODO: Implement error handling
        // Handle IR parsing errors
        std::cerr << "IR Error: " << e.what() << std::endl;
        return 1;
        
    } catch (const std::exception& e) {
        // TODO: Implement general error handling
        // Handle other errors (file I/O, etc.)
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}