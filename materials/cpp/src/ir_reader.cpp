#include "ir.hpp"

#include <fstream>
#include <sstream>
#include <regex>
#include <cctype>

using namespace ircpp;

static bool isConstantToken(const std::string& s) {
    static const std::regex kNum(R"(^-?\d+(?:\.\d*)?$)");
    return std::regex_match(s, kNum);
}

IRProgram IRReader::parseIRFile(const std::string& filename) const {
    std::ifstream in(filename);
    if (!in) throw IRException("File not found: " + filename);

    struct IRLine { int lineNumber; std::string line; };
    std::vector<IRLine> irLines;

    std::string raw;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        std::string s;
        s.reserve(raw.size());
        size_t b = raw.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) continue;
        size_t e = raw.find_last_not_of(" \t\r\n");
        s = raw.substr(b, e - b + 1);
        if (!s.empty()) irLines.push_back({lineNo, s});
    }

    auto toUpper = [](std::string x){ for (auto& c : x) c = (char)toupper(c); return x; };

    auto parseType = [&](const std::string& typeStr, int /*ln*/) -> std::shared_ptr<IRType> {
        static const std::regex pat(R"(^(?:(void)|(?:(int|float)(?:\[(\d+)\])?))$)");
        std::smatch m;
        if (!std::regex_match(typeStr, m, pat)) throw IRException("Invalid type");
        if (m[1].matched) return nullptr;
        std::shared_ptr<IRType> elem;
        if (m[2].str() == "int") elem = IRIntType::get();
        else if (m[2].str() == "float") elem = IRFloatType::get();
        else throw IRException("Invalid type");
        if (!m[3].matched) return elem;
        int size = std::stoi(m[3].str());
        if (size <= 0) throw IRException("Invalid array size");
        return IRArrayType::get(elem, size);
    };

    auto getDataType = [&](const std::shared_ptr<IROperand>& x) -> std::shared_ptr<IRType> {
        if (auto c = std::dynamic_pointer_cast<IRConstantOperand>(x)) return c->type;
        if (auto v = std::dynamic_pointer_cast<IRVariableOperand>(x)) return v->type;
        return nullptr;
    };

    auto parseFunction = [&](const std::vector<IRLine>& lines) -> std::shared_ptr<IRFunction> {
        std::unordered_map<std::string, std::shared_ptr<IRVariableOperand>> variableMap;
        size_t idx = 1;
        const auto& sig = lines.at(idx++);
        std::string sigLine = sig.line;
        for (char& c : sigLine) if (c == '(' || c == ')' || c == ',' || c == ':') c = ' ';
        std::stringstream ss(sigLine);
        std::vector<std::string> tok; std::string t;
        while (ss >> t) tok.push_back(t);
        if (tok.size() < 2 || (tok.size() % 2) != 0) throw IRException("Invalid function signature");

        auto retType = parseType(tok[0], sig.lineNumber);
        if (std::dynamic_pointer_cast<IRArrayType>(retType)) throw IRException("Invalid type");
        std::string fnName = tok[1];

        std::vector<std::shared_ptr<IRVariableOperand>> params;
        for (size_t i = 2; i < tok.size(); i += 2) {
            auto pType = parseType(tok[i], sig.lineNumber);
            if (!pType) throw IRException("Invalid type");
            const std::string& pName = tok[i + 1];
            if (!std::regex_match(pName, std::regex(R"(^[A-Za-z_][A-Za-z0-9_]*$)")))
                throw IRException("Invalid parameter name");
            if (variableMap.count(pName)) throw IRException("Redefinition of variable");
            auto p = std::make_shared<IRVariableOperand>(pType, pName, nullptr);
            variableMap[pName] = p;
            params.push_back(p);
        }

        const auto& intListLine = lines.at(idx++);
        const auto& floatListLine = lines.at(idx++);
        auto parseVarList = [&](const IRLine& vline, std::shared_ptr<IRType> elementType) {
            auto colon = vline.line.find(':');
            std::string rest = (colon == std::string::npos) ? std::string() : vline.line.substr(colon + 1);
            size_t b = rest.find_first_not_of(" \t");
            if (b == std::string::npos) return;
            rest = rest.substr(b);
            std::stringstream ssv(rest);
            std::string name;
            while (std::getline(ssv, name, ',')) {
                size_t bb = name.find_first_not_of(" \t");
                size_t ee = name.find_last_not_of(" \t");
                if (bb == std::string::npos) continue;
                name = name.substr(bb, ee - bb + 1);
                std::smatch m;
                static const std::regex arrPat(R"(^(.+)\[(\d+)\]$)");
                if (std::regex_match(name, m, arrPat)) {
                    int size = std::stoi(m[2].str());
                    if (size <= 0) throw IRException("Invalid array size");
                    std::string base = m[1].str();
                    if (!std::regex_match(base, std::regex(R"(^[A-Za-z_][A-Za-z0-9_]*$)")))
                        throw IRException("Invalid variable name");
                    auto tarr = IRArrayType::get(elementType, size);
                    if (variableMap.count(base)) throw IRException("Redefinition of variable");
                    variableMap[base] = std::make_shared<IRVariableOperand>(tarr, base, nullptr);
                } else {
                    if (!std::regex_match(name, std::regex(R"(^[A-Za-z_][A-Za-z0-9_]*$)")))
                        throw IRException("Invalid variable name");
                    if (variableMap.count(name)) throw IRException("Redefinition of variable");
                    variableMap[name] = std::make_shared<IRVariableOperand>(elementType, name, nullptr);
                }
            }
        };

        parseVarList(intListLine, IRIntType::get());
        parseVarList(floatListLine, IRFloatType::get());

        std::vector<std::shared_ptr<IRInstruction>> insts;
        auto makeConstOrVar = [&](IRInstruction* parent, const std::string& tok) -> std::shared_ptr<IROperand> {
            if (isConstantToken(tok)) {
                if (tok.find('.') != std::string::npos) return std::make_shared<IRConstantOperand>(IRFloatType::get(), tok, parent);
                return std::make_shared<IRConstantOperand>(IRIntType::get(), tok, parent);
            }
            auto it = variableMap.find(tok);
            if (it == variableMap.end()) throw IRException("Variable used without definition");
            return std::make_shared<IRVariableOperand>(it->second->type, it->second->getName(), parent);
        };

        while (idx < lines.size()) {
            const auto& l = lines.at(idx++);
            if (!l.line.empty() && l.line[0] == '#') break;

            auto inst = std::make_shared<IRInstruction>();
            inst->irLineNumber = l.lineNumber;

            if (!l.line.empty() && l.line.back() == ':') {
                std::string label = l.line.substr(0, l.line.size() - 1);
                inst->opCode = IRInstruction::OpCode::LABEL;
                inst->operands.push_back(std::make_shared<IRLabelOperand>(label, inst.get()));
                insts.push_back(inst);
                continue;
            }

            std::vector<std::string> tokens;
            {
                std::string tmp = l.line;
                for (char& c : tmp) if (c == ',') c = ' ';
                std::stringstream sst(tmp);
                std::string x;
                while (sst >> x) tokens.push_back(x);
            }
            if (tokens.empty()) continue;
            std::string opUpper = toUpper(tokens[0]);
            auto parseOp = [&](const std::string& s)->IRInstruction::OpCode{
                if (s=="ASSIGN") return IRInstruction::OpCode::ASSIGN;
                if (s=="ADD") return IRInstruction::OpCode::ADD;
                if (s=="SUB") return IRInstruction::OpCode::SUB;
                if (s=="MULT") return IRInstruction::OpCode::MULT;
                if (s=="DIV") return IRInstruction::OpCode::DIV;
                if (s=="AND") return IRInstruction::OpCode::AND;
                if (s=="OR") return IRInstruction::OpCode::OR;
                if (s=="GOTO") return IRInstruction::OpCode::GOTO;
                if (s=="BREQ") return IRInstruction::OpCode::BREQ;
                if (s=="BRNEQ") return IRInstruction::OpCode::BRNEQ;
                if (s=="BRLT") return IRInstruction::OpCode::BRLT;
                if (s=="BRGT") return IRInstruction::OpCode::BRGT;
                if (s=="BRGEQ") return IRInstruction::OpCode::BRGEQ;
                if (s=="RETURN") return IRInstruction::OpCode::RETURN;
                if (s=="CALL") return IRInstruction::OpCode::CALL;
                if (s=="CALLR") return IRInstruction::OpCode::CALLR;
                if (s=="ARRAY_STORE") return IRInstruction::OpCode::ARRAY_STORE;
                if (s=="ARRAY_LOAD") return IRInstruction::OpCode::ARRAY_LOAD;
                throw IRException("Invalid OpCode");
            };
            inst->opCode = parseOp(opUpper);

            auto getType = [&](const std::shared_ptr<IROperand>& x){ return getDataType(x); };

            switch (inst->opCode) {
                case IRInstruction::OpCode::ASSIGN: {
                    if (tokens.size() > 3) {
                        inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                        inst->operands.push_back(makeConstOrVar(inst.get(), tokens[2]));
                        inst->operands.push_back(makeConstOrVar(inst.get(), tokens[3]));
                        auto t0 = getType(inst->operands[0]);
                        auto t1 = getType(inst->operands[1]);
                        auto t2 = getType(inst->operands[2]);
                        auto arr = std::dynamic_pointer_cast<IRArrayType>(t0);
                        if (!(arr && t1.get() == IRIntType::get().get() && arr->elementType.get() == t2.get()))
                            throw IRException("Invalid operand");
                    } else {
                        inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                        inst->operands.push_back(makeConstOrVar(inst.get(), tokens[2]));
                        auto t0 = getType(inst->operands[0]);
                        auto t1 = getType(inst->operands[1]);
                        if (std::dynamic_pointer_cast<IRArrayType>(t0) || t0.get() != t1.get())
                            throw IRException("Invalid operand");
                    }
                    break;
                }
                case IRInstruction::OpCode::ADD:
                case IRInstruction::OpCode::SUB:
                case IRInstruction::OpCode::MULT:
                case IRInstruction::OpCode::DIV:
                case IRInstruction::OpCode::AND:
                case IRInstruction::OpCode::OR: {
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[2]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[3]));
                    auto t0 = getType(inst->operands[0]);
                    auto t1 = getType(inst->operands[1]);
                    auto t2 = getType(inst->operands[2]);
                    if (!(std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]) &&
                          !std::dynamic_pointer_cast<IRArrayType>(t0) && t0.get()==t1.get() && t1.get()==t2.get()))
                        throw IRException("Invalid operand");
                    break;
                }
                case IRInstruction::OpCode::GOTO: {
                    inst->operands.push_back(std::make_shared<IRLabelOperand>(tokens[1], inst.get()));
                    break;
                }
                case IRInstruction::OpCode::BREQ:
                case IRInstruction::OpCode::BRNEQ:
                case IRInstruction::OpCode::BRLT:
                case IRInstruction::OpCode::BRGT:
                case IRInstruction::OpCode::BRGEQ: {
                    inst->operands.push_back(std::make_shared<IRLabelOperand>(tokens[1], inst.get()));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[2]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[3]));
                    auto t1 = getType(inst->operands[1]);
                    auto t2 = getType(inst->operands[2]);
                    if (std::dynamic_pointer_cast<IRArrayType>(t1) || t1.get() != t2.get())
                        throw IRException("Invalid operand");
                    break;
                }
                case IRInstruction::OpCode::RETURN: {
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                    auto t0 = getType(inst->operands[0]);
                    if (std::dynamic_pointer_cast<IRArrayType>(t0)) throw IRException("Invalid operand");
                    break;
                }
                case IRInstruction::OpCode::CALL: {
                    inst->operands.push_back(std::make_shared<IRFunctionOperand>(tokens[1], inst.get()));
                    for (size_t i = 2; i < tokens.size(); ++i) inst->operands.push_back(makeConstOrVar(inst.get(), tokens[i]));
                    break;
                }
                case IRInstruction::OpCode::CALLR: {
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                    inst->operands.push_back(std::make_shared<IRFunctionOperand>(tokens[2], inst.get()));
                    for (size_t i = 3; i < tokens.size(); ++i) inst->operands.push_back(makeConstOrVar(inst.get(), tokens[i]));
                    auto t0 = getType(inst->operands[0]);
                    if (!(std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]) && !std::dynamic_pointer_cast<IRArrayType>(t0)))
                        throw IRException("Invalid operand");
                    break;
                }
                case IRInstruction::OpCode::ARRAY_STORE: {
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[2]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[3]));
                    auto t0 = getType(inst->operands[0]);
                    auto t1 = getType(inst->operands[1]);
                    auto t2 = getType(inst->operands[2]);
                    auto arr = std::dynamic_pointer_cast<IRArrayType>(t1);
                    if (!(!std::dynamic_pointer_cast<IRArrayType>(t0) && arr && t2.get()==IRIntType::get().get() && arr->elementType.get()==t0.get()))
                        throw IRException("Invalid operand");
                    break;
                }
                case IRInstruction::OpCode::ARRAY_LOAD: {
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[1]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[2]));
                    inst->operands.push_back(makeConstOrVar(inst.get(), tokens[3]));
                    auto t0 = getType(inst->operands[0]);
                    auto t1 = getType(inst->operands[1]);
                    auto t2 = getType(inst->operands[2]);
                    auto arr = std::dynamic_pointer_cast<IRArrayType>(t1);
                    if (!(std::dynamic_pointer_cast<IRVariableOperand>(inst->operands[0]) && !std::dynamic_pointer_cast<IRArrayType>(t0) && arr && t2.get()==IRIntType::get().get() && arr->elementType.get()==t0.get()))
                        throw IRException("Invalid operand");
                    break;
                }
                default: throw IRException("Invalid OpCode");
            }

            insts.push_back(inst);
        }

        std::vector<std::shared_ptr<IRVariableOperand>> vars;
        vars.reserve(variableMap.size());
        for (auto& kv : variableMap) vars.push_back(kv.second);

        return std::make_shared<IRFunction>(fnName, retType, params, vars, insts);
    };

    std::vector<std::shared_ptr<IRFunction>> functions;
    std::vector<IRLine> buf;
    for (size_t i = 0; i < irLines.size(); ++i) {
        const auto& L = irLines[i];
        if (L.line.rfind("#start_function", 0) == 0) {
            if (!buf.empty()) throw IRException("Unexpected #start_function");
            buf.push_back(L);
        } else if (L.line.rfind("#end_function", 0) == 0) {
            if (buf.empty()) throw IRException("Unexpected #end_function");
            buf.push_back(L);
            functions.push_back(parseFunction(buf));
            buf.clear();
        } else {
            buf.push_back(L);
        }
    }

    IRProgram p;
    p.functions = std::move(functions);
    return p;
}


