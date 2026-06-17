#include <cstdio>
#include <cstdlib>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

static const char* typeName(const std::variant<int64_t, double, std::string, bool>& v) {
    if (std::holds_alternative<int64_t>(v)) return "int";
    if (std::holds_alternative<double>(v)) return "float";
    if (std::holds_alternative<std::string>(v)) return "str";
    if (std::holds_alternative<bool>(v)) return "bool";
    return "?";
}

void dumpAST(const UCLang::NodePtr& node, int depth = 0) {
    auto indent = [depth]() { for (int i = 0; i < depth; ++i) printf("  "); };
    std::visit(UCLang::Visitor{
        [&](const UCLang::ProgramNode& n) {
            indent(); printf("Program (%zu stmts):\n", n.stmts.size());
            for (size_t i = 0; i < n.stmts.size(); ++i) {
                indent(); printf("  [%zu]:\n", i);
                dumpAST(n.stmts[i], depth + 2);
            }
        },
        [&](const UCLang::FuncDefNode& n) {
            indent(); printf("FuncDef '%s' params=[", n.name.c_str());
            for (size_t i = 0; i < n.params.size(); ++i) {
                if (i) printf(",");
                printf("%s", n.params[i].c_str());
            }
            printf("] body=%zu stmts\n", n.body.size());
            for (auto& s : n.body) dumpAST(s, depth + 1);
        },
        [&](const UCLang::FuncCallNode& n) {
            indent(); printf("FuncCall '%s' args=%zu\n", n.name.c_str(), n.args.size());
        },
        [&](const UCLang::PrintNode& n) {
            indent(); printf("Print\n");
        },
        [&](const UCLang::IfNode& n) {
            indent(); printf("If\n");
        },
        [&](const UCLang::AssignNode& n) {
            indent(); printf("Assign '%s'\n", n.var.c_str());
        },
        [&](const UCLang::TypeCheckNode& n) {
            indent(); printf("TypeCheck '%s'\n", n.typeName.c_str());
        },
        [&](const UCLang::LiteralNode& n) {
            indent(); printf("Literal (%s)\n", typeName(n.value));
        },
        [&](const UCLang::IdentifierNode& n) {
            indent(); printf("Identifier '%s'\n", n.name.c_str());
        },
        [&](const UCLang::BinaryOpNode& n) {
            indent(); printf("BinaryOp '%s'\n", n.op.c_str());
        },
        [&](const UCLang::ReturnNode& n) {
            indent(); printf("Return\n");
        },
        [&](const UCLang::InputNode& n) {
            indent(); printf("Input\n");
        },
        [&](const UCLang::HtmlNode& n) {
            indent(); printf("Html\n");
        },
        [&](std::monostate) {
            indent(); printf("(empty)\n");
        }
    }, node->data);
}

int main() {
    FILE* f = fopen("test.uc", "rb");
    if (!f) { printf("cannot open test.uc\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* src = (char*)malloc((size_t)len + 1);
    fread(src, 1, (size_t)len, f);
    src[len] = 0;
    fclose(f);
    
    printf("=== SOURCE ===\n%s\n", src);
    
    UCLang::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    
    printf("\n=== TOKENS (%zu) ===\n", tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        printf("  [%zu] type=%d val='%s' line=%d col=%d\n",
               i, (int)tokens[i].type, tokens[i].value.c_str(),
               tokens[i].line, tokens[i].column);
    }
    
    UCLang::Parser parser(tokens);
    auto ast = parser.parse();
    
    printf("\n=== AST ===\n");
    dumpAST(ast);
    
    free(src);
    return 0;
}
