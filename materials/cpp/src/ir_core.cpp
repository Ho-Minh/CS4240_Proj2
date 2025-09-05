#include "ir.hpp"

using namespace ircpp;

static const char* opToString(IRInstruction::OpCode op) {
    switch (op) {
        case IRInstruction::OpCode::ASSIGN: return "assign";
        case IRInstruction::OpCode::ADD: return "add";
        case IRInstruction::OpCode::SUB: return "sub";
        case IRInstruction::OpCode::MULT: return "mult";
        case IRInstruction::OpCode::DIV: return "div";
        case IRInstruction::OpCode::AND: return "and";
        case IRInstruction::OpCode::OR: return "or";
        case IRInstruction::OpCode::GOTO: return "goto";
        case IRInstruction::OpCode::BREQ: return "breq";
        case IRInstruction::OpCode::BRNEQ: return "brneq";
        case IRInstruction::OpCode::BRLT: return "brlt";
        case IRInstruction::OpCode::BRGT: return "brgt";
        case IRInstruction::OpCode::BRGEQ: return "brgeq";
        case IRInstruction::OpCode::RETURN: return "return";
        case IRInstruction::OpCode::CALL: return "call";
        case IRInstruction::OpCode::CALLR: return "callr";
        case IRInstruction::OpCode::ARRAY_STORE: return "array_store";
        case IRInstruction::OpCode::ARRAY_LOAD: return "array_load";
        case IRInstruction::OpCode::LABEL: return "label";
    }
    return "";
}

void IRPrinter::printProgram(const IRProgram& program) const {
    for (const auto& fptr : program.functions) {
        printFunction(*fptr);
        os << '\n';
    }
}

static std::string typeToString(const std::shared_ptr<IRType>& t) {
    if (!t) return "void";
    if (t.get() == IRIntType::get().get()) return "int";
    if (t.get() == IRFloatType::get().get()) return "float";
    auto arr = std::dynamic_pointer_cast<IRArrayType>(t);
    if (arr) {
        return typeToString(arr->elementType) + "[" + std::to_string(arr->size) + "]";
    }
    return "";
}

void IRPrinter::printFunction(const IRFunction& function) const {
    os << "#start_function\n";
    os << (function.returnType ? typeToString(function.returnType) : std::string("void"));
    os << ' ' << function.name << '(';
    bool first = true;
    for (const auto& p : function.parameters) {
        if (!first) os << ", ";
        first = false;
        os << typeToString(p->type) << ' ' << p->getName();
    }
    os << "):\n";

    // variable lists
    std::vector<std::string> intList;
    std::vector<std::string> floatList;
    for (const auto& v : function.variables) {
        auto arr = std::dynamic_pointer_cast<IRArrayType>(v->type);
        if (arr) {
            std::string entry = v->getName() + "[" + std::to_string(arr->size) + "]";
            if (arr->elementType.get() == IRIntType::get().get()) intList.push_back(entry);
            else floatList.push_back(entry);
        } else {
            if (v->type.get() == IRIntType::get().get()) intList.push_back(v->getName());
            else floatList.push_back(v->getName());
        }
    }
    os << "int-list: ";
    for (size_t i = 0; i < intList.size(); ++i) {
        if (i) os << ", ";
        os << intList[i];
    }
    os << "\nfloat-list: ";
    for (size_t i = 0; i < floatList.size(); ++i) {
        if (i) os << ", ";
        os << floatList[i];
    }
    os << "\n";

    for (const auto& inst : function.instructions) {
        if (inst->opCode != IRInstruction::OpCode::LABEL) os << "    ";
        printInstruction(*inst);
    }
    os << "#end_function\n";
}

void IRPrinter::printInstruction(const IRInstruction& instruction) const {
    if (instruction.opCode == IRInstruction::OpCode::LABEL) {
        os << instruction.operands[0]->toString() << ":\n";
        return;
    }
    os << opToString(instruction.opCode);
    for (const auto& op : instruction.operands) {
        os << ", " << op->toString();
    }
    os << '\n';
}

int IRInterpreterStats::getNonLabelInstructionCount() const {
    auto it = instructionCounts.find(IRInstruction::OpCode::LABEL);
    int labelCount = (it == instructionCounts.end()) ? 0 : it->second;
    return totalInstructionCount - labelCount;
}


