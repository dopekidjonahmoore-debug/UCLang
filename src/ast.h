#pragma once
#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace UCLang {

struct Node;
using NodePtr = std::unique_ptr<Node>;

struct ProgramNode     { std::vector<NodePtr> stmts; };
struct FuncDefNode     { std::string name; std::vector<std::string> params; std::vector<NodePtr> body; };
struct FuncCallNode    { std::string name; std::vector<NodePtr> args; };
struct IfNode          { NodePtr cond; std::vector<NodePtr> thenBody; std::vector<NodePtr> elseBody; };
struct WhileNode       { NodePtr cond; std::vector<NodePtr> body; };
struct BreakNode       { };
struct NextNode        { };
struct PrintNode       { NodePtr value; };
struct InputNode       { NodePtr prompt; std::string responseVar; };
struct AssignNode      { std::string var; NodePtr value; };
struct TypeCheckNode   { NodePtr expr; std::string typeName; };
struct BinaryOpNode    { std::string op; NodePtr left; NodePtr right; };
struct LiteralNode     { std::variant<int64_t, double, std::string, bool> value; };
struct IdentifierNode  { std::string name; };
struct ListLiteralNode { std::vector<NodePtr> elements; };
struct ListGetNode     { NodePtr list; NodePtr index; };
struct HtmlNode        { std::string rawHtml; };
struct ReturnNode      { NodePtr value; };
struct MultiAssignNode { std::vector<std::string> vars; NodePtr value; };
struct GameLoopNode    { std::vector<NodePtr> updateBody; std::vector<NodePtr> drawBody; };

// ── OOP nodes ──────────────────────────────────────────────
struct ClassMemberNode { std::string visibility; bool isStatic; bool isOverride; std::string name; NodePtr value; };
struct ClassDefNode    { std::string name; std::string parent; std::vector<std::string> interfaces; std::vector<ClassMemberNode> members; };
struct NewExprNode     { std::string className; std::vector<NodePtr> args; };
struct ThisExprNode    { };
struct SuperExprNode   { };
struct MemberCallNode  { NodePtr object; std::string member; std::vector<NodePtr> args; };
struct MemberGetNode   { NodePtr object; std::string member; };
struct MemberSetNode   { NodePtr object; std::string member; NodePtr value; };
struct NullNode        { };
struct YieldNode       { NodePtr value; };
struct ImportNode      { std::string path; };
struct AsCastNode      { NodePtr expr; std::string typeName; };

struct Node {
    std::variant<
        std::monostate,
        ProgramNode,
        FuncDefNode,
        FuncCallNode,
        IfNode,
        WhileNode,
        BreakNode,
        NextNode,
        PrintNode,
        InputNode,
        AssignNode,
        TypeCheckNode,
        BinaryOpNode,
        LiteralNode,
        IdentifierNode,
        ListLiteralNode,
        ListGetNode,
        HtmlNode,
        ReturnNode,
        MultiAssignNode,
        GameLoopNode,
        ClassDefNode,
        NewExprNode,
        ThisExprNode,
        SuperExprNode,
        MemberCallNode,
        MemberGetNode,
        MemberSetNode,
        NullNode,
        YieldNode,
        ImportNode,
        AsCastNode
    > data;

    template<typename T, typename... Args>
    static NodePtr make(Args&&... args) {
        auto n = std::make_unique<Node>();
        n->data = T{std::forward<Args>(args)...};
        return n;
    }
};

template<typename... Fs>
struct Visitor : Fs... { using Fs::operator()...; };
template<typename... Fs>
Visitor(Fs...) -> Visitor<Fs...>;

} // namespace UCLang
