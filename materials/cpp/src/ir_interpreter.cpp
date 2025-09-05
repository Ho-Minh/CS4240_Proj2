#include "ir.hpp"

#include <stack>
#include <variant>
#include <sstream>

using namespace ircpp;

namespace {
// Use a non-recursive variant. Arrays are strongly typed as vector<int> or vector<float>.
using Value = std::variant<int, float, std::vector<int>, std::vector<float>>;

static int asInt(const Value& v) { return std::get<int>(v); }
static float asFloat(const Value& v) { return std::get<float>(v); }

static bool isIntType(const std::shared_ptr<IRType>& t) { return t.get() == IRIntType::get().get(); }
static bool isFloatType(const std::shared_ptr<IRType>& t) { return t.get() == IRFloatType::get().get(); }
}

struct StackFrame {
    std::shared_ptr<IRFunction> caller;
    std::shared_ptr<IRInstruction> callInst;
    int returnInstIdx{0};
    std::shared_ptr<IRFunction> function;
    std::unordered_map<std::string, Value> varMap;
};

struct ProgramCounter {
    std::vector<std::shared_ptr<IRInstruction>>* currentInstList{nullptr};
    int nextIdx{0};
    void set(std::vector<std::shared_ptr<IRInstruction>>* list, int idx) { currentInstList = list; nextIdx = idx; }
    void setNextIdx(int idx) { nextIdx = idx; }
    int getNextIdx() const { return nextIdx; }
    std::shared_ptr<IRInstruction> next() { return (*currentInstList)[nextIdx++]; }
    bool hasNext() const { return nextIdx < (int)currentInstList->size(); }
};

IRInterpreter::IRInterpreter(const std::string& filename) {
    IRReader r;
    program = r.parseIRFile(filename);
}

static Value getConstVal(const IRConstantOperand& c) {
    if (isIntType(c.type)) return std::stoi(c.getValueString());
    return std::stof(c.getValueString());
}

static Value getValFromVarOrConst(const std::shared_ptr<IROperand>& op, const StackFrame& sf) {
    if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(op)) {
        auto it = sf.varMap.find(v->getName());
        return it->second;
    }
    return getConstVal(*std::dynamic_pointer_cast<IRConstantOperand>(op));
}

static Value binaryOperation(IRInstruction::OpCode op, const std::shared_ptr<IRType>& type, const Value& a, const Value& b) {
    if (isIntType(type)) {
        int iy = asInt(a); int iz = asInt(b);
        switch (op) {
            case IRInstruction::OpCode::ADD: return iy + iz;
            case IRInstruction::OpCode::SUB: return iy - iz;
            case IRInstruction::OpCode::MULT: return iy * iz;
            case IRInstruction::OpCode::DIV: return iy / iz;
            case IRInstruction::OpCode::AND: return iy & iz;
            case IRInstruction::OpCode::OR:  return iy | iz;
            case IRInstruction::OpCode::BREQ: return (int)(iy == iz);
            case IRInstruction::OpCode::BRNEQ: return (int)(iy != iz);
            case IRInstruction::OpCode::BRLT: return (int)(iy < iz);
            case IRInstruction::OpCode::BRGT: return (int)(iy > iz);
            case IRInstruction::OpCode::BRGEQ: return (int)(iy >= iz);
            default: break;
        }
    } else {
        float fy = asFloat(a); float fz = asFloat(b);
        switch (op) {
            case IRInstruction::OpCode::ADD: return fy + fz;
            case IRInstruction::OpCode::SUB: return fy - fz;
            case IRInstruction::OpCode::MULT: return fy * fz;
            case IRInstruction::OpCode::DIV: return fy / fz;
            case IRInstruction::OpCode::BREQ: return (int)(fy == fz);
            case IRInstruction::OpCode::BRNEQ: return (int)(fy != fz);
            case IRInstruction::OpCode::BRLT: return (int)(fy < fz);
            case IRInstruction::OpCode::BRGT: return (int)(fy > fz);
            case IRInstruction::OpCode::BRGEQ: return (int)(fy >= fz);
            default: break;
        }
    }
    throw IRException("Unsupported binary operation");
}

void IRInterpreter::run() {
    std::unordered_map<std::string, std::shared_ptr<IRFunction>> functionMap;
    std::unordered_map<std::shared_ptr<IRFunction>, std::unordered_map<std::string, int>> functionLabelMap;
    for (auto& f : program.functions) {
        functionMap[f->name] = f;
        std::unordered_map<std::string, int> labels;
        for (int i = 0; i < (int)f->instructions.size(); ++i) {
            if (f->instructions[i]->opCode == IRInstruction::OpCode::LABEL) {
                auto label = std::dynamic_pointer_cast<IRLabelOperand>(f->instructions[i]->operands[0]);
                labels[label->getName()] = i;
            }
        }
        functionLabelMap[f] = std::move(labels);
    }

    auto entryCall = std::make_shared<IRInstruction>(IRInstruction::OpCode::CALL, std::vector<std::shared_ptr<IROperand>>{ std::make_shared<IRFunctionOperand>("main", nullptr) }, -1);
    std::vector<std::shared_ptr<IRInstruction>> entryInstList{ entryCall };
    ProgramCounter pc; pc.set(&entryInstList, 0);

    std::stack<StackFrame> stack;
    StackFrame entrySF; stack.push(entrySF);

    stats.totalInstructionCount = -1;
    stats.instructionCounts[IRInstruction::OpCode::CALL] = -1;

    auto buildVarMap = [&](const std::shared_ptr<IRFunction>& f, const std::vector<Value>& args) {
        std::unordered_map<std::string, Value> vm;
        for (const auto& v : f->variables) {
            if (auto arr = std::dynamic_pointer_cast<IRArrayType>(v->type)) {
                if (isIntType(arr->elementType)) {
                    vm[v->getName()] = std::vector<int>(arr->size, 0);
                } else {
                    vm[v->getName()] = std::vector<float>(arr->size, 0.0f);
                }
            } else if (isIntType(v->type)) {
                vm[v->getName()] = 0;
            } else {
                vm[v->getName()] = 0.0f;
            }
        }
        for (size_t i = 0; i < f->parameters.size(); ++i) vm[f->parameters[i]->getName()] = args[i];
        return vm;
    };

    auto getValFromVarOrConst2 = [&](const std::shared_ptr<IROperand>& o, const StackFrame& sf) -> Value { return getValFromVarOrConst(o, sf); };

    auto executeCall = [&](const std::shared_ptr<IRInstruction>& callInst, const std::shared_ptr<IRFunction>& fn, const std::vector<Value>& args, ProgramCounter& pcRef, std::stack<StackFrame>& st) {
        StackFrame& callerSF = st.top();
        StackFrame calleeSF;
        calleeSF.caller = callerSF.function;
        calleeSF.callInst = callInst;
        calleeSF.returnInstIdx = pcRef.getNextIdx();
        calleeSF.function = fn;
        calleeSF.varMap = buildVarMap(fn, args);
        st.push(std::move(calleeSF));
        pcRef.set(&fn->instructions, 0);
    };

    auto handleIntrinsic = [&](const std::shared_ptr<IRInstruction>& callInst, const std::string& name, const std::vector<Value>& args, std::stack<StackFrame>& st) {
        if (name == "geti") {
            int i = 0; std::string line; std::getline(std::cin, line); if (!line.empty()) { std::stringstream ss(line); ss >> i; }
            auto retVar = std::dynamic_pointer_cast<IRVariableOperand>(callInst->operands[0]);
            st.top().varMap[retVar->getName()] = i;
        } else if (name == "getf") {
            float f = 0.0f; std::string line; std::getline(std::cin, line); if (!line.empty()) { std::stringstream ss(line); ss >> f; }
            auto retVar = std::dynamic_pointer_cast<IRVariableOperand>(callInst->operands[0]);
            st.top().varMap[retVar->getName()] = f;
        } else if (name == "getc") {
            int c = std::cin.get();
            auto retVar = std::dynamic_pointer_cast<IRVariableOperand>(callInst->operands[0]);
            st.top().varMap[retVar->getName()] = c;
        } else if (name == "puti") {
            std::cout << asInt(args[0]);
        } else if (name == "putf") {
            std::cout << asFloat(args[0]);
        } else if (name == "putc") {
            std::cout << (char)asInt(args[0]);
        } else {
            throw IRException("Undefined reference to function '" + name + "'");
        }
    };

    auto throwRuntime = [&](const std::shared_ptr<IRInstruction>& inst, const std::string& msg, std::stack<StackFrame>& st) -> void {
        std::cerr << "IR interpreter runtime exception: " << msg << "\n";
        std::cerr << "Stack trace:\n";
        if (!st.empty()) {
            std::cerr << "\t" << st.top().function->name << ":" << inst->irLineNumber << "\n";
        }
        throw IRException(msg);
    };

    auto executeInstruction = [&](const std::shared_ptr<IRInstruction>& inst, ProgramCounter& pcRef, std::stack<StackFrame>& st) {
        stats.totalInstructionCount += 1;
        stats.instructionCounts[inst->opCode] += 1;

        StackFrame& sf = st.top();
        switch (inst->opCode) {
            case IRInstruction::OpCode::ASSIGN: {
                if (inst->operands.size() > 2) { // array assignment
                    auto destVar = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]);
                    auto arrType = std::dynamic_pointer_cast<IRArrayType>(destVar->type);
                    int assignSize = asInt(getValFromVarOrConst2(inst->operands[1], sf));
                    Value src = getValFromVarOrConst2(inst->operands[2], sf);
                    Value& arrVal = sf.varMap[destVar->getName()];
                    if (isIntType(arrType->elementType)) {
                        auto& arr = std::get<std::vector<int>>(arrVal);
                        if (assignSize < 0 || assignSize > (int)arr.size()) throwRuntime(inst, "Out-of-bounds array access", st);
                        int s = asInt(src);
                        for (int i = 0; i < assignSize; ++i) arr[i] = s;
                    } else {
                        auto& arr = std::get<std::vector<float>>(arrVal);
                        if (assignSize < 0 || assignSize > (int)arr.size()) throwRuntime(inst, "Out-of-bounds array access", st);
                        float s = asFloat(src);
                        for (int i = 0; i < assignSize; ++i) arr[i] = s;
                    }
                } else {
                    auto dest = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]);
                    Value src = getValFromVarOrConst2(inst->operands[1], sf);
                    sf.varMap[dest->getName()] = src;
                }
                break;
            }
            case IRInstruction::OpCode::ADD:
            case IRInstruction::OpCode::SUB:
            case IRInstruction::OpCode::MULT:
            case IRInstruction::OpCode::DIV:
            case IRInstruction::OpCode::AND:
            case IRInstruction::OpCode::OR: {
                auto dest = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]);
                Value y = getValFromVarOrConst2(inst->operands[1], sf);
                Value z = getValFromVarOrConst2(inst->operands[2], sf);
                auto type = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0])->type;
                Value x = binaryOperation(inst->opCode, type, y, z);
                sf.varMap[dest->getName()] = x;
                break;
            }
            case IRInstruction::OpCode::GOTO: {
                auto label = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0])->getName();
                auto& map = functionLabelMap[sf.function];
                pcRef.setNextIdx(map[label]);
                break;
            }
            case IRInstruction::OpCode::BREQ:
            case IRInstruction::OpCode::BRNEQ:
            case IRInstruction::OpCode::BRLT:
            case IRInstruction::OpCode::BRGT:
            case IRInstruction::OpCode::BRGEQ: {
                auto label = std::dynamic_pointer_cast<IRLabelOperand>(inst->operands[0])->getName();
                Value a = getValFromVarOrConst2(inst->operands[1], sf);
                Value b = getValFromVarOrConst2(inst->operands[2], sf);
                auto type = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[1])->type;
                int result = asInt(binaryOperation(inst->opCode, type, a, b));
                if (result) pcRef.setNextIdx(functionLabelMap[sf.function][label]);
                break;
            }
            case IRInstruction::OpCode::RETURN: {
                Value retVal = getValFromVarOrConst2(inst->operands[0], sf);
                auto caller = sf.caller;
                auto callInst = sf.callInst;
                st.pop();
                StackFrame& callerSF = st.top();
                auto retVar = std::dynamic_pointer_cast<IRVariableOperand>(callInst->operands[0]);
                callerSF.varMap[retVar->getName()] = retVal;
                pcRef.set(&caller->instructions, sf.returnInstIdx);
                break;
            }
            case IRInstruction::OpCode::CALL:
            case IRInstruction::OpCode::CALLR: {
                std::vector<Value> args;
                size_t startIdx = (inst->opCode == IRInstruction::OpCode::CALL) ? 1 : 2;
                for (size_t i = startIdx; i < inst->operands.size(); ++i)
                    args.push_back(getValFromVarOrConst2(inst->operands[i], sf));
                std::string calleeName = std::dynamic_pointer_cast<IRFunctionOperand>(inst->operands[(inst->opCode == IRInstruction::OpCode::CALL) ? 0 : 1])->getName();
                if (functionMap.count(calleeName)) {
                    executeCall(inst, functionMap[calleeName], args, pcRef, st);
                } else {
                    handleIntrinsic(inst, calleeName, args, st);
                }
                break;
            }
            case IRInstruction::OpCode::ARRAY_STORE: {
                auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[1]);
                auto arrType = std::dynamic_pointer_cast<IRArrayType>(arrVar->type);
                int offset = asInt(getValFromVarOrConst2(inst->operands[2], sf));
                Value src = getValFromVarOrConst2(inst->operands[0], sf);
                Value& arrVal = sf.varMap[arrVar->getName()];
                if (isIntType(arrType->elementType)) {
                    auto& arr = std::get<std::vector<int>>(arrVal);
                    if (offset < 0 || offset >= (int)arr.size()) throwRuntime(inst, "Out-of-bounds array access", st);
                    arr[offset] = asInt(src);
                } else {
                    auto& arr = std::get<std::vector<float>>(arrVal);
                    if (offset < 0 || offset >= (int)arr.size()) throwRuntime(inst, "Out-of-bounds array access", st);
                    arr[offset] = asFloat(src);
                }
                break;
            }
            case IRInstruction::OpCode::ARRAY_LOAD: {
                auto dest = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]);
                auto arrVar = std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[1]);
                auto arrType = std::dynamic_pointer_cast<IRArrayType>(arrVar->type);
                int offset = asInt(getValFromVarOrConst2(inst->operands[2], sf));
                Value& arrVal = sf.varMap[arrVar->getName()];
                if (isIntType(arrType->elementType)) {
                    auto& arr = std::get<std::vector<int>>(arrVal);
                    if (offset < 0 || offset >= (int)arr.size()) throwRuntime(inst, "Out-of-bounds array access", st);
                    sf.varMap[dest->getName()] = arr[offset];
                } else {
                    auto& arr = std::get<std::vector<float>>(arrVal);
                    if (offset < 0 || offset >= (int)arr.size()) throwRuntime(inst, "Out-of-bounds array access", st);
                    sf.varMap[dest->getName()] = arr[offset];
                }
                break;
            }
            case IRInstruction::OpCode::LABEL: break;
        }
    };

    while (true) {
        auto instruction = pc.next();
        executeInstruction(instruction, pc, stack);
        if (!pc.hasNext()) {
            StackFrame sf = stack.top(); stack.pop();
            if (stack.size() == 1) break;
            auto caller = sf.caller;
            pc.set(&caller->instructions, sf.returnInstIdx);
        }
    }
}


