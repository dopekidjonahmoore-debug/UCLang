#pragma once
#include <string>
#include "ast.h"

namespace UCLang {

class Compiler {
public:
    // Compile a UCLang AST to standalone C source code.
    std::string compile(const Node& node);

private:
    std::string m_code;
    std::string m_funcDecls;
    std::string m_curEnv;  // current environment variable name ("env" or "&_env")
    int m_tmpCounter = 0;

    void compileStmt(const Node& node);
    void compileStmts(const std::vector<NodePtr>& stmts);
    void compileProgram(const ProgramNode& n);
    std::string compileExpr(const Node& node);
    std::string tempVar(const char* prefix);
};

} // namespace UCLang
