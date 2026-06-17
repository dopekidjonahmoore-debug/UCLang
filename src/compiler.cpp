#include "compiler.h"
#include <unordered_set>
#include <sstream>

namespace UCLang {

// ============================================================
//  Compiler
// ============================================================

std::string Compiler::compile(const Node& node) {
    m_code.clear();
    m_funcDecls.clear();
    m_tmpCounter = 0;

    m_code += "#include \"ucl_runtime.h\"\n\n";

    // First pass: collect function definitions
    std::unordered_set<std::string> funcNames;
    std::vector<const FuncDefNode*> funcDefs;
    if (auto* prog = std::get_if<ProgramNode>(&node.data)) {
        for (const auto& stmt : prog->stmts) {
            if (auto* fd = std::get_if<FuncDefNode>(&stmt->data)) {
                funcNames.insert(fd->name);
                funcDefs.push_back(fd);
            }
        }
    }

    // Native-skip set: these functions are provided by the runtime
    std::unordered_set<std::string> nativeFns = {"is_digit", "is_alpha", "is_alnum"};

    for (const auto& fd : funcDefs) {
        if (nativeFns.count(fd->name)) continue;
        m_funcDecls += "static Value func_" + fd->name + "(Env* env";
        for (size_t i = 0; i < fd->params.size(); i++)
            m_funcDecls += ", Value p" + std::to_string(i);
        m_funcDecls += ");\n";
    }
    if (!funcDefs.empty()) m_code += m_funcDecls + "\n";

    // Function definitions
    for (const auto& fd : funcDefs) {
        if (nativeFns.count(fd->name)) continue;
        m_code += "static Value func_" + fd->name + "(Env* env";
        for (size_t i = 0; i < fd->params.size(); i++)
            m_code += ", Value p" + std::to_string(i);
        m_code += ") {\n";

        m_curEnv = "env";
        for (size_t i = 0; i < fd->params.size(); i++)
            m_code += "    env_set(env, \"" + fd->params[i] + "\", p" + std::to_string(i) + ");\n";

        if (fd->name == "tokenize" || fd->name == "parse" || fd->name == "generate" || fd->name == "compiler")
            m_code += "    fprintf(stderr, \"" + fd->name + " start\\n\"); fflush(stderr);\n";

        compileStmts(fd->body);
        m_code += "    return val_null();\n";
        m_code += "}\n\n";
    }

    // Main function
    m_curEnv = "&_env";
    m_code += "int main(void) {\n";
    m_code += "    Env _env = {0};\n";

    if (auto* prog = std::get_if<ProgramNode>(&node.data)) {
        for (const auto& stmt : prog->stmts) {
            if (std::get_if<FuncDefNode>(&stmt->data)) continue;
            compileStmt(*stmt);
        }
    }

    m_code += "    return 0;\n}\n";
    return m_code;
}

void Compiler::compileStmt(const Node& node) {
    std::visit(Visitor{
        [&](const ProgramNode& n)   { compileProgram(n); },
        [&](const PrintNode& n)     {
            m_code += "    ucl_print(" + compileExpr(*n.value) + ");\n";
        },
        [&](const AssignNode& n)    {
            m_code += "    env_set(" + m_curEnv + ", \"" + n.var + "\", " + compileExpr(*n.value) + ");\n";
        },
        [&](const IfNode& n)        {
            m_code += "    if (val_truthy(" + compileExpr(*n.cond) + ")) {\n";
            compileStmts(n.thenBody);
            m_code += "    }";
            if (!n.elseBody.empty()) {
                m_code += " else {\n";
                compileStmts(n.elseBody);
                m_code += "    }";
            }
            m_code += "\n";
        },
        [&](const WhileNode& n)     {
            m_code += "    while (val_truthy(" + compileExpr(*n.cond) + ")) {\n";
            compileStmts(n.body);
            m_code += "    }\n";
        },
        [&](const BreakNode&)       { m_code += "    break;\n"; },
        [&](const NextNode&)        { m_code += "    continue;\n"; },
        [&](const ReturnNode& n)    {
            m_code += "    return " + compileExpr(*n.value) + ";\n";
        },
        [&](const FuncDefNode&)     { },
        [&](const FuncCallNode& n)  {
            m_code += "    " + compileExpr(node) + ";\n";
        },
        [&](const InputNode& n)     {
            m_code += "    env_set(" + m_curEnv + ", \"" + n.responseVar + "\", " + compileExpr(*n.prompt) + ");\n";
        },
        [&](const MultiAssignNode& n) {
            std::string tv = tempVar("ma");
            m_code += "    Value " + tv + " = " + compileExpr(*n.value) + ";\n";
            for (size_t i = 0; i < n.vars.size(); i++)
                m_code += "    env_set(" + m_curEnv + ", \"" + n.vars[i] + "\", val_copy(" + tv + ".list.items[" + std::to_string(i) + "]));\n";
        },
        [&](const auto&)            { },
    }, node.data);
}

void Compiler::compileStmts(const std::vector<NodePtr>& stmts) {
    for (const auto& stmt : stmts) {
        // Handle InputNode specially
        if (auto* in = std::get_if<InputNode>(&stmt->data)) {
            m_code += "    env_set(" + m_curEnv + ", \"" + in->responseVar + "\", ucl_input(" + compileExpr(*in->prompt) + "));\n";
        } else {
            compileStmt(*stmt);
        }
    }
}

void Compiler::compileProgram(const ProgramNode& n) {
    for (const auto& stmt : n.stmts) compileStmt(*stmt);
}

std::string Compiler::tempVar(const char* prefix) {
    return std::string("_") + prefix + std::to_string(m_tmpCounter++);
}

static std::string escStr(const std::string& v) {
    std::string esc;
    for (char c : v) {
        if (c == '\\') esc += "\\\\";
        else if (c == '"') esc += "\\\"";
        else if (c == '\n') esc += "\\n";
        else if (c == '\t') esc += "\\t";
        else if (c >= 32) esc += c;
        else { char buf[16]; snprintf(buf, 16, "\\x%02x", (unsigned char)c); esc += buf; }
    }
    return esc;
}

static std::string opToFunc(const std::string& op) {
    if (op == "+")  return "val_add";
    if (op == "-")  return "val_sub";
    if (op == "*")  return "val_mul";
    if (op == "/")  return "val_div";
    if (op == "=")  return "val_eq";
    if (op == "!=") return "val_neq";
    if (op == ">")  return "val_gt";
    if (op == "<")  return "val_lt";
    if (op == "=>") return "val_gte";
    if (op == "<=") return "val_lte";
    if (op == "or") return "val_or";
    if (op == "and") return "val_and";
    return "val_null";
}

std::string Compiler::compileExpr(const Node& node) {
    return std::visit(Visitor{
        [&](const BinaryOpNode& n) -> std::string {
            return opToFunc(n.op) + "(" + compileExpr(*n.left) + ", " + compileExpr(*n.right) + ")";
        },
        [&](const LiteralNode& n) -> std::string {
            return std::visit(Visitor{
                [&](int64_t v) -> std::string          { return "val_int(" + std::to_string(v) + ")"; },
                [&](double v) -> std::string           { return "val_float(" + std::to_string(v) + ")"; },
                [&](const std::string& v) -> std::string { return "val_string(\"" + escStr(v) + "\")"; },
                [&](bool v) -> std::string             { return v ? "val_bool(1)" : "val_bool(0)"; },
            }, n.value);
        },
        [&](const IdentifierNode& n) -> std::string {
            return "env_get(" + m_curEnv + ", \"" + n.name + "\")";
        },
        [&](const ListLiteralNode& n) -> std::string {
            std::string vl = tempVar("vl");
            std::string rv = tempVar("lv");
            m_code += "    ValueList " + vl + " = {NULL,0,0};\n";
            for (const auto& e : n.elements)
                m_code += "    list_push_val(&" + vl + ", " + compileExpr(*e) + ");\n";
            m_code += "    Value " + rv + " = {VAL_LIST, 0, 0, NULL, 0, " + vl + "};\n";
            return rv;
        },
        [&](const ListGetNode& n) -> std::string {
            std::string lv = tempVar("lg");
            std::string iv = tempVar("li");
            m_code += "    Value " + lv + " = " + compileExpr(*n.list) + ";\n";
            m_code += "    Value " + iv + " = " + compileExpr(*n.index) + ";\n";
            return "val_copy(" + lv + ".list.items[" + iv + ".i])";
        },
        [&](const FuncCallNode& n) -> std::string {
            bool isBuiltin = (n.name == "list_push" || n.name == "list_pop" || n.name == "list_len" || n.name == "list_get" ||
                             n.name == "file_read" || n.name == "file_write" ||
                             n.name == "str_len" || n.name == "str_get" || n.name == "str_slice" ||
                             n.name == "int_to_str" || n.name == "str_to_int");
            if (isBuiltin) {
                std::string av = tempVar("a");
                m_code += "    Value " + av + "[" + std::to_string(n.args.size()) + "];\n";
                for (size_t i = 0; i < n.args.size(); i++)
                    m_code += "    " + av + "[" + std::to_string(i) + "] = " + compileExpr(*n.args[i]) + ";\n";
                return "builtin_" + n.name + "(" + std::to_string(n.args.size()) + ", " + av + ")";
            }
            std::string r = "func_" + n.name + "(" + m_curEnv;
            for (const auto& a : n.args) r += ", " + compileExpr(*a);
            return r + ")";
        },
        [&](const PrintNode& n) -> std::string {
            std::string pv = tempVar("pr");
            m_code += "    Value " + pv + " = " + compileExpr(*n.value) + ";\n";
            m_code += "    ucl_print(" + pv + ");\n";
            return pv;
        },
        [&](const InputNode& n) -> std::string {
            return "ucl_input(" + compileExpr(*n.prompt) + ")";
        },
        [&](const auto&) -> std::string { return "val_null()"; },
    }, node.data);
}

} // namespace UCLang
