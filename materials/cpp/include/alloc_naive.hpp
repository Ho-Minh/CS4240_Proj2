#pragma once

#include <vector>
#include "ir.hpp"
#include "mips_instructions.hpp"

namespace ircpp {

// Emit a full function (prologue, body, epilogue) using naive per-instruction
// load/compute/store allocation.
std::vector<MIPSInstruction> emitFunctionNaive(const IRFunction& F);

}


