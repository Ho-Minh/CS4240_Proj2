// Unified C++ IR header translated from Java classes in materials/src
// Provides declarations for IR datatypes, operands, core IR, reader, printer, and interpreter

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

namespace ircpp {

// Forward decls
struct IRInstruction;

// Datatypes
struct IRType {
    virtual ~IRType() = default;
};

struct IRIntType : public IRType {
    static std::shared_ptr<IRIntType> get();
};

struct IRFloatType : public IRType {
    static std::shared_ptr<IRFloatType> get();
};

struct IRArrayType : public IRType {
    std::shared_ptr<IRType> elementType;
    int size;
    static std::shared_ptr<IRArrayType> get(std::shared_ptr<IRType> elementType, int size);
private:
    IRArrayType(std::shared_ptr<IRType> elementType, int size);
};

// Operands
struct IROperand {
    std::string value;
    IRInstruction* parent;
    explicit IROperand(std::string v, IRInstruction* p) : value(std::move(v)), parent(p) {}
    virtual ~IROperand() = default;
    virtual std::string toString() const { return value; }
};

struct IRConstantOperand : public IROperand {
    std::shared_ptr<IRType> type;
    IRConstantOperand(std::shared_ptr<IRType> t, const std::string& v, IRInstruction* p)
        : IROperand(v, p), type(std::move(t)) {}
    std::string getValueString() const { return value; }
};

struct IRFunctionOperand : public IROperand {
    IRFunctionOperand(const std::string& name, IRInstruction* p) : IROperand(name, p) {}
    std::string getName() const { return value; }
};

struct IRLabelOperand : public IROperand {
    IRLabelOperand(const std::string& name, IRInstruction* p) : IROperand(name, p) {}
    std::string getName() const { return value; }
};

struct IRVariableOperand : public IROperand {
    std::shared_ptr<IRType> type;
    IRVariableOperand(std::shared_ptr<IRType> t, const std::string& name, IRInstruction* p)
        : IROperand(name, p), type(std::move(t)) {}
    std::string getName() const { return value; }
};

// Core IR
struct IRInstruction {
    enum class OpCode {
        ASSIGN,
        ADD, SUB, MULT, DIV, AND, OR,
        GOTO,
        BREQ, BRNEQ, BRLT, BRGT, BRGEQ,
        RETURN,
        CALL, CALLR,
        ARRAY_STORE, ARRAY_LOAD,
        LABEL
    };

    OpCode opCode{};
    std::vector<std::shared_ptr<IROperand>> operands;
    int irLineNumber{};

    IRInstruction() = default;
    IRInstruction(OpCode code, std::vector<std::shared_ptr<IROperand>> ops, int line)
        : opCode(code), operands(std::move(ops)), irLineNumber(line) {}
};

struct IRFunction;

struct IRProgram {
    std::vector<std::shared_ptr<IRFunction>> functions;
};

struct IRFunction {
    std::string name;
    std::shared_ptr<IRType> returnType; // nullptr for void
    std::vector<std::shared_ptr<IRVariableOperand>> parameters;
    std::vector<std::shared_ptr<IRVariableOperand>> variables;
    std::vector<std::shared_ptr<IRInstruction>> instructions;

    IRFunction(std::string n,
               std::shared_ptr<IRType> ret,
               std::vector<std::shared_ptr<IRVariableOperand>> params,
               std::vector<std::shared_ptr<IRVariableOperand>> vars,
               std::vector<std::shared_ptr<IRInstruction>> insts)
        : name(std::move(n)), returnType(std::move(ret)), parameters(std::move(params)),
          variables(std::move(vars)), instructions(std::move(insts)) {}
};

// Exceptions
struct IRException : public std::runtime_error {
    explicit IRException(const std::string& s) : std::runtime_error(s) {}
};

// Printer
struct IRPrinter {
    std::ostream& os;
    explicit IRPrinter(std::ostream& out) : os(out) {}
    void printProgram(const IRProgram& program) const;
    void printFunction(const IRFunction& function) const;
    void printInstruction(const IRInstruction& instruction) const;
};

// Reader (parser)
struct IRReader {
    IRProgram parseIRFile(const std::string& filename) const; // May throw IRException
};

// Interpreter
struct IRInterpreterStats {
    int totalInstructionCount{0};
    std::unordered_map<IRInstruction::OpCode, int> instructionCounts;
    int getNonLabelInstructionCount() const;
};

struct IRInterpreter {
    explicit IRInterpreter(const std::string& filename);
    void run();
    const IRInterpreterStats& getStats() const { return stats; }
private:
    IRProgram program;
    IRInterpreterStats stats;
};

// Control Flow Graph structures
struct BasicBlock {
    std::string id;
    std::vector<std::shared_ptr<IRInstruction>> instructions;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;
    
    BasicBlock(const std::string& blockId) : id(blockId) {}
};

struct ControlFlowGraph {
    std::unordered_map<std::string, std::shared_ptr<BasicBlock>> blocks;
    std::string entryBlock;
    std::vector<std::string> exitBlocks;
    
    void addBlock(std::shared_ptr<BasicBlock> block) {
        blocks[block->id] = block;
    }
    
    void addEdge(const std::string& from, const std::string& to) {
        if (blocks.count(from) && blocks.count(to)) {
            blocks[from]->successors.push_back(to);
            blocks[to]->predecessors.push_back(from);
        }
    }
};

struct CFGBuilder {
    static ControlFlowGraph buildCFG(const IRFunction& function);
    static void printCFG(const ControlFlowGraph& cfg, std::ostream& os);
    static void printCFGDot(const ControlFlowGraph& cfg, std::ostream& os);
private:
    static std::vector<std::shared_ptr<BasicBlock>> identifyBasicBlocks(const IRFunction& function);
    static void buildEdges(ControlFlowGraph& cfg, const std::vector<std::shared_ptr<BasicBlock>>& blocks);
};

} // namespace ircpp


