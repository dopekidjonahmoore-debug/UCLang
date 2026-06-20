#pragma once
#include "ast.h"
#include <variant>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace UCLang {

struct BreakSignal {};
struct NextSignal  {};
struct CoroutineData;

// Recursive variant: Value can hold a list of Values.
struct Value;
struct ValueList : std::vector<Value> {};

typedef glm::vec3  Vec3;
typedef glm::quat  Quat;
typedef glm::mat4  Mat4;

struct UCLangObjectData {
    std::string className;
    std::unordered_map<std::string, Value> fields;
};

struct Value : std::variant<std::monostate, int64_t, double, std::string, bool, ValueList, std::shared_ptr<UCLangObjectData>, Vec3, Quat, Mat4, std::shared_ptr<CoroutineData>> {
    using variant::variant;
};

struct YieldSignal { Value value; };

struct CoroutineData {
    std::string funcName;
    const FuncDefNode* funcDef;
    std::vector<std::string> paramNames;
    std::vector<Value> argValues;
    int yieldStep = 0;
    bool done = false;
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

    // OOP exec methods
    Value execClassDef(const ClassDefNode& n);
    Value execNewExpr(const NewExprNode& n);
    Value execMemberCall(const MemberCallNode& n);
    Value execMemberGet(const MemberGetNode& n);
    Value execMemberSet(const MemberSetNode& n);
    Value execImport(const ImportNode& n);
    Value execYield(const YieldNode& n);

    // Access control helpers
    bool checkAccess(const std::string& fieldDefiningClass, const ClassMemberNode* member) const;
    const ClassDefNode* getCurrentClassDef() const;
    const ClassDefNode* findFieldDefiningClass(const ClassDefNode* startClass, const std::string& fieldName) const;
    const ClassDefNode* findMethodDefiningClass(const ClassDefNode* startClass, const std::string& methodName) const;

    void execStmts(const std::vector<NodePtr>& stmts);

    Value evaluateExpr(const Node& node);

    // OOP helpers
    const ClassDefNode* findClass(const std::string& name) const;
    const ClassMemberNode* findMethodInClass(const ClassDefNode* cls, const std::string& methodName) const;

    std::shared_ptr<Environment> m_env;
    std::unordered_map<std::string, const FuncDefNode*> m_functions;
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>> m_nativeFuncs;
    std::unordered_map<std::string, const ClassDefNode*> m_classes;
    std::string m_currentThisClassName;    // runtime class of `this` object
    std::string m_currentDefiningClassName; // class that DEFINES the currently executing method
    Value m_currentThis;
    std::vector<NodePtr> m_modules;
};

} // namespace UCLang
