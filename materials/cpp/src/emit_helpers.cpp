#include "emit_helpers.hpp"

namespace ircpp {

void emitLoadOperand(const FrameInfo& fi,
                     const std::shared_ptr<IROperand>& op,
                     const std::shared_ptr<Register>& dst,
                     std::vector<MIPSInstruction>& code) {
    if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(op)) {
        int val = std::stoi(c->getValueString());
        code.emplace_back(MIPSOp::LI, "", std::vector<std::shared_ptr<MIPSOperand>>{ dst, std::make_shared<Immediate>(val) });
        return;
    }
    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
        int off = fi.varOffset.at(v->getName());
        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ dst, std::make_shared<Address>(off, Registers::fp()) });
        return;
    }
    // labels/functions not supported here
}

void emitStoreVar(const FrameInfo& fi,
                  const std::string& name,
                  const std::shared_ptr<Register>& src,
                  std::vector<MIPSInstruction>& code) {
    int off = fi.varOffset.at(name);
    code.emplace_back(MIPSOp::SW, "", std::vector<std::shared_ptr<MIPSOperand>>{ src, std::make_shared<Address>(off, Registers::fp()) });
}

void emitComputeArrayAddr(const FrameInfo& fi,
                          const std::string& arrayName,
                          const std::shared_ptr<Register>& indexReg,
                          const std::shared_ptr<Register>& addrReg,
                          const std::shared_ptr<Register>& baseReg,
                          std::vector<MIPSInstruction>& code) {
    int baseOff = fi.varOffset.at(arrayName);
    if (fi.paramArrayNames.count(arrayName)) {
        // load pointer from slot
        code.emplace_back(MIPSOp::LW, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, std::make_shared<Address>(baseOff, Registers::fp()) });
    } else {
        // compute frame base address
        code.emplace_back(MIPSOp::ADDI, "", std::vector<std::shared_ptr<MIPSOperand>>{ baseReg, Registers::fp(), std::make_shared<Immediate>(baseOff) });
    }
    // addr = base + (idx<<2)
    code.emplace_back(MIPSOp::SLL, "", std::vector<std::shared_ptr<MIPSOperand>>{ addrReg, indexReg, std::make_shared<Immediate>(2) });
    code.emplace_back(MIPSOp::ADD, "", std::vector<std::shared_ptr<MIPSOperand>>{ addrReg, baseReg, addrReg });
}

void emitStoreF32(const FrameInfo& fi,
                  const std::string& name,
                  const std::shared_ptr<Register>& fSrc,
                  std::vector<MIPSInstruction>& code) {
    int off = fi.varOffset.at(name);
    code.emplace_back(MIPSOp::S_S, "", std::vector<std::shared_ptr<MIPSOperand>>{ fSrc, std::make_shared<Address>(off, Registers::fp()) });
}

void emitLoadF32(const FrameInfo& fi,
                 const std::shared_ptr<IROperand>& op,
                 const std::shared_ptr<Register>& fDst,
                 std::vector<MIPSInstruction>& code) {
    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
        int off = fi.varOffset.at(v->getName());
        code.emplace_back(MIPSOp::L_S, "", std::vector<std::shared_ptr<MIPSOperand>>{ fDst, std::make_shared<Address>(off, Registers::fp()) });
    }
}

} // namespace ircpp


