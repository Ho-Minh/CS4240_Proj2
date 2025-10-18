#include "mips_instructions.hpp"
#include <sstream>

using namespace std;

namespace ircpp {

std::string opToString(MIPSOp op) {
    switch (op) {
        case MIPSOp::ADD: return "add";
        case MIPSOp::ADDI: return "addi";
        case MIPSOp::SUB: return "sub";
        case MIPSOp::MUL: return "mul";
        case MIPSOp::DIV: return "div";
        case MIPSOp::AND: return "and";
        case MIPSOp::ANDI: return "andi";
        case MIPSOp::OR: return "or";
        case MIPSOp::ORI: return "ori";
        case MIPSOp::SLL: return "sll";
        case MIPSOp::LI: return "li";
        case MIPSOp::LW: return "lw";
        case MIPSOp::MOVE: return "move";
        case MIPSOp::SW: return "sw";
        case MIPSOp::LA: return "la";
        case MIPSOp::BEQ: return "beq";
        case MIPSOp::BNE: return "bne";
        case MIPSOp::BLT: return "blt";
        case MIPSOp::BGT: return "bgt";
        case MIPSOp::BGE: return "bge";
        case MIPSOp::J: return "j";
        case MIPSOp::JAL: return "jal";
        case MIPSOp::JR: return "jr";
        case MIPSOp::SYSCALL: return "syscall";
        case MIPSOp::ADD_S: return "add.s";
        case MIPSOp::ADDI_S: return "addi.s";
        case MIPSOp::SUB_S: return "sub.s";
        case MIPSOp::MUL_S: return "mul.s";
        case MIPSOp::DIV_S: return "div.s";
        case MIPSOp::LI_S: return "li.s";
        case MIPSOp::MOV_S: return "mov.s";
        case MIPSOp::L_S: return "l.s";
        case MIPSOp::S_S: return "s.s";
        case MIPSOp::C_EQ_S: return "c.eq.s";
        case MIPSOp::C_NE_S: return "c.ne.s";
        case MIPSOp::C_LT_S: return "c.lt.s";
        case MIPSOp::C_GT_S: return "c.gt.s";
        case MIPSOp::C_GE_S: return "c.ge.s";
        case MIPSOp::BC1T: return "bc1t";
        case MIPSOp::BC1F: return "bc1f";
        case MIPSOp::MFLO: return "mflo";
        default: return "unknown";
    }
}

std::string MIPSInstruction::toString() const {
    string ans = "";
    
    // TODO: Implement MIPS instruction string formatting
    // Format: [label:] opcode operand1, operand2, operand3
    // Examples:
    //   "add $t0, $t1, $t2"
    //   "loop: beq $t0, $t1, exit"
    //   "li $t0, 42"
    
    // Implementation steps:
    // 1. If label exists, add "label: " prefix
    // 2. Add opcode name (use MIPSOp enum to string conversion)
    // 3. Add operands separated by commas
    // 4. Handle special cases (no operands, single operand, etc.)

    if (label != "") {
        ans = label + ": ";
    }
    ans = ans + opToString(op);
    if (!operands.empty()) ans = ans + " ";
    for (int i = 0; i < (int)operands.size(); i++) {
        if (i > 0) ans = ans + ", ";
        ans = ans + operands[i]->toString();
    }
    ans = ans + "\n";
    return ans;
}
} // namespace ircpp
