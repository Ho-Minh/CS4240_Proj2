#pragma once

#include <string>
#include <vector>
#include <memory>

namespace ircpp {

// Forward declarations
struct MIPSOperand;
struct Register;
struct Immediate;
struct Address;

// MIPS32 instruction opcodes supported by the interpreter
enum class MIPSOp {
    // Arithmetic operations
    ADD, ADDI, SUB, MUL, DIV, AND, ANDI, OR, ORI, SLL,
    
    // Data movement
    LI, LW, MOVE, SW, LA,
    
    // Control flow
    BEQ, BNE, BLT, BGT, BGE, J, JAL, JR,
    
    // System
    SYSCALL,
    
    // Floating point (for future extension)
    ADD_S, ADDI_S, SUB_S, MUL_S, DIV_S,
    LI_S, MOV_S, L_S, S_S,
    C_EQ_S, C_NE_S, C_LT_S, C_GT_S, C_GE_S,
    BC1T, BC1F
};

// Base class for MIPS operands
struct MIPSOperand {
    virtual ~MIPSOperand() = default;
    virtual std::string toString() const = 0;
};

// Register operand (e.g., $t0, $a0, $sp)
struct Register : public MIPSOperand {
    std::string name;  // e.g., "t0", "a0", "sp"
    bool isPhysical;   // true for $t0, $a0, etc.; false for virtual regs
    
    Register(const std::string& regName, bool physical = true) 
        : name(regName), isPhysical(physical) {}
    
    std::string toString() const override {
        return "$" + name;
    }
};

// Immediate value operand (e.g., 42, -5)
struct Immediate : public MIPSOperand {
    int value;
    
    Immediate(int val) : value(val) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

// Memory address operand (e.g., 0($sp), 4($fp))
struct Address : public MIPSOperand {
    int offset;
    std::shared_ptr<Register> base;
    
    Address(int off, std::shared_ptr<Register> baseReg) 
        : offset(off), base(baseReg) {}
    
    std::string toString() const override {
        return std::to_string(offset) + "(" + base->toString() + ")";
    }
};

// Label operand for branches and jumps
struct Label : public MIPSOperand {
    std::string name;
    
    Label(const std::string& labelName) : name(labelName) {}
    
    std::string toString() const override {
        return name;
    }
};

// MIPS instruction representation
struct MIPSInstruction {
    MIPSOp op;
    std::string label;  // Optional label for this instruction
    std::vector<std::shared_ptr<MIPSOperand>> operands;
    
    MIPSInstruction(MIPSOp operation, const std::string& lbl = "", 
                   std::vector<std::shared_ptr<MIPSOperand>> ops = {})
        : op(operation), label(lbl), operands(std::move(ops)) {}
    
    std::string toString() const;
};

// Helper function to create common registers
namespace Registers {
    // Physical MIPS registers
    inline auto t0() { return std::make_shared<Register>("t0"); }
    inline auto t1() { return std::make_shared<Register>("t1"); }
    inline auto t2() { return std::make_shared<Register>("t2"); }
    inline auto t3() { return std::make_shared<Register>("t3"); }
    inline auto t4() { return std::make_shared<Register>("t4"); }
    inline auto t5() { return std::make_shared<Register>("t5"); }
    inline auto t6() { return std::make_shared<Register>("t6"); }
    inline auto t7() { return std::make_shared<Register>("t7"); }
    inline auto t8() { return std::make_shared<Register>("t8"); }
    inline auto t9() { return std::make_shared<Register>("t9"); }
    
    inline auto s0() { return std::make_shared<Register>("s0"); }
    inline auto s1() { return std::make_shared<Register>("s1"); }
    inline auto s2() { return std::make_shared<Register>("s2"); }
    inline auto s3() { return std::make_shared<Register>("s3"); }
    inline auto s4() { return std::make_shared<Register>("s4"); }
    inline auto s5() { return std::make_shared<Register>("s5"); }
    inline auto s6() { return std::make_shared<Register>("s6"); }
    inline auto s7() { return std::make_shared<Register>("s7"); }
    
    inline auto a0() { return std::make_shared<Register>("a0"); }
    inline auto a1() { return std::make_shared<Register>("a1"); }
    inline auto a2() { return std::make_shared<Register>("a2"); }
    inline auto a3() { return std::make_shared<Register>("a3"); }
    
    inline auto v0() { return std::make_shared<Register>("v0"); }
    inline auto v1() { return std::make_shared<Register>("v1"); }
    
    inline auto sp() { return std::make_shared<Register>("sp"); }
    inline auto fp() { return std::make_shared<Register>("fp"); }
    inline auto ra() { return std::make_shared<Register>("ra"); }
    inline auto zero() { return std::make_shared<Register>("zero"); }
}

// Helper function to convert MIPSOp enum to string
std::string opToString(MIPSOp op);

} // namespace ircpp
