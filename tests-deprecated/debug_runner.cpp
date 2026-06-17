#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

int main(int argc, char* argv[]) {
    printf("A\n"); fflush(stdout);
    const char* file = (argc > 1) ? argv[1] : "compiler.uc";
    std::ifstream f(file);
    if (!f.is_open()) { printf("Cannot open file: %s\n", file); return 1; }
    std::stringstream buf; buf << f.rdbuf();
    std::string src = buf.str();
    printf("B: src=%zu\n", src.size()); fflush(stdout);

    try {
        UCLang::Lexer lexer(src);
        auto tokens = lexer.tokenize();
        printf("C: tokens=%zu\n", tokens.size()); fflush(stdout);
        printf("Tokens: %zu\n", tokens.size());

        UCLang::Parser parser(tokens);
        auto ast = parser.parse();
        printf("D: parse done\n"); fflush(stdout);
        auto& prog = std::get<UCLang::ProgramNode>(ast->data);
        printf("Top-level stmts: %zu\n", prog.stmts.size());
        int funcDefs = 0, others = 0;
        for (auto& s : prog.stmts) {
            if (std::get_if<UCLang::FuncDefNode>(&s->data)) funcDefs++;
            else {
                others++;
                const char* tn = "?";
                if (std::get_if<UCLang::FuncCallNode>(&s->data)) tn = "FuncCall";
                else if (std::get_if<UCLang::BinaryOpNode>(&s->data)) tn = "BinaryOp";
                else if (std::get_if<UCLang::LiteralNode>(&s->data)) tn = "Literal";
                else if (std::get_if<UCLang::IdentifierNode>(&s->data)) tn = "Ident";
                printf("  Other[%d]: %s\n", others, tn);
            }
        }
        printf("  FuncDefs: %d, Others: %d\n", funcDefs, others);

        UCLang::Interpreter interp;
        interp.registerBuiltins();
        std::string output;
        interp.outputFn = [&](const std::string& s) { output += s; };
        printf("E: starting execute\n"); fflush(stdout);
        interp.execute(*ast);
        printf("F: execute done\n"); fflush(stdout);
        printf("Output: |%s|\n", output.c_str());
    } catch (const std::exception& e) {
        printf("ERROR: %s\n", e.what());
    }
    return 0;
}
