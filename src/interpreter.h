#pragma once
#include "ast.h"
#include <variant>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace UCLang {

struct BreakSignal {};
struct NextSignal  {};

// Recursive variant: Value can hold a list of Values.
// ValueList is a separate type that holds the vector to break the cycle.
struct Value;
struct ValueList : std::vector<Value> {};
struct Value : std::variant<std::monostate, int64_t, double, std::string, bool, ValueList> {
    using variant::variant;
};

class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment(std::shared_ptr<Environment> parent = nullptr);

    Value get(const std::string& name) const;
    void  set(const std::string& name, const Value& v);
    void  setOrUpdate(const std::string& name, const Value& v);

private:
    std::unordered_map<std::string, Value> m_vars;
    std::shared_ptr<Environment> m_parent;
};

class Interpreter {
public:
    Interpreter();

    std::function<std::string(const std::string& prompt)> inputFn;
    std::function<void(const std::string& text)>          outputFn;
    std::function<void(const std::string& html)>          htmlFn;

    void registerFunction(const FuncDefNode& func);
    void registerBuiltins();

    Value execute(const Node& node);

    const std::unordered_map<std::string, const FuncDefNode*>& functions() const { return m_functions; }

private:
    Value executeNode(const Node& node);
    Value execProgram(const ProgramNode& n);
    Value execFuncDef(const FuncDefNode& n);
    Value execFuncCall(const FuncCallNode& n, std::shared_ptr<Environment> env);
    Value execIf(const IfNode& n);
    Value execWhile(const WhileNode& n);
    Value execPrint(const PrintNode& n);
    Value execInput(const InputNode& n);
    Value execAssign(const AssignNode& n);
    Value execMultiAssign(const MultiAssignNode& n);
    Value execTypeCheck(const TypeCheckNode& n);
    Value execBinaryOp(const BinaryOpNode& n);
    Value execLiteral(const LiteralNode& n);
    Value execIdentifier(const IdentifierNode& n);
    Value execListLiteral(const ListLiteralNode& n);
    Value execListGet(const ListGetNode& n);
    Value execHtml(const HtmlNode& n);
    Value execReturn(const ReturnNode& n);
    Value execGameLoop(const GameLoopNode& n);

    void execStmts(const std::vector<NodePtr>& stmts);

    Value evaluateExpr(const Node& node);

    std::shared_ptr<Environment> m_env;
    std::unordered_map<std::string, const FuncDefNode*> m_functions;
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>> m_nativeFuncs;
    std::vector<NodePtr> m_modules; // keep parsed modules alive (for run_file)
};

} // namespace UCLang
