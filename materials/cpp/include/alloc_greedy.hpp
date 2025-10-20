#pragma once

#include <vector>
#include "ir.hpp"
#include "mips_instructions.hpp"

namespace ircpp {

// Emit a full function (prologue, body, epilogue) using intra-block greedy
// allocation with per-block loads/stores and spills on control transfers.
std::vector<MIPSInstruction> emitFunctionGreedy(const IRFunction& F);

}


