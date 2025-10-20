#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mips_instructions.hpp"
#include "ir.hpp"
#include "frame_builder.hpp"

namespace ircpp {

// Load integer/label/array operand into a general-purpose register.
// - Constants -> li dst, imm
// - Scalar variables -> lw dst, off($fp)
// - Array variables -> lw dst, off($fp) (loads base pointer for params)
void emitLoadOperand(const FrameInfo& fi,
                     const std::shared_ptr<IROperand>& op,
                     const std::shared_ptr<Register>& dst,
                     std::vector<MIPSInstruction>& code);

// Store general-purpose register to a scalar variable slot
void emitStoreVar(const FrameInfo& fi,
                  const std::string& name,
                  const std::shared_ptr<Register>& src,
                  std::vector<MIPSInstruction>& code);

// Compute element address for arrayName[indexReg] into addrReg using baseReg as temp
void emitComputeArrayAddr(const FrameInfo& fi,
                          const std::string& arrayName,
                          const std::shared_ptr<Register>& indexReg,
                          const std::shared_ptr<Register>& addrReg,
                          const std::shared_ptr<Register>& baseReg,
                          std::vector<MIPSInstruction>& code);

// Float helpers
void emitStoreF32(const FrameInfo& fi,
                  const std::string& name,
                  const std::shared_ptr<Register>& fSrc,
                  std::vector<MIPSInstruction>& code);

void emitLoadF32(const FrameInfo& fi,
                 const std::shared_ptr<IROperand>& op,
                 const std::shared_ptr<Register>& fDst,
                 std::vector<MIPSInstruction>& code);

} // namespace ircpp


