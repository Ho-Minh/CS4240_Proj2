#include "mips_instructions.hpp"
#include <sstream>

namespace ircpp {

std::string MIPSInstruction::toString() const {
    std::ostringstream oss;
    
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
    
    return oss.str();
}

} // namespace ircpp
