#pragma once
#include "ast.h"
#include "lexer.h"
#include <vector>

namespace UCLang {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    NodePtr parse();

private:
    std::vector<Token> m_tokens;
    size_t m_pos = 0;

    const Token& peek(int offset = 0) const;
    Token advance();
    bool atEnd() const;
    Token expect(TokenType type, const std::string& msg = "");
    bool match(TokenType type);
    Token previous() const;

    NodePtr parseProgram();

    NodePtr parseStatement();
    NodePtr parseFuncDef();
    NodePtr parseFuncCall();
    NodePtr parseFuncCallExpr(const Token& nameTok);
    NodePtr parseIf();
    NodePtr parseWhile();
    NodePtr parseBreak();
    NodePtr parseNext();
    NodePtr parsePrint();
    NodePtr parseInput();
    NodePtr parseHtml();
    NodePtr parseReturn();
    NodePtr parseGameLoop();
    NodePtr parseNamespaceCall(const Token& ident, bool consumeStmtEnd = true);
    NodePtr parseAssign(const Token& ident);
    NodePtr parseMultiAssign(const Token& ident);
    NodePtr parseTypeCheck(const Token& ident);

    NodePtr parseExpr();
    NodePtr parseBinaryOp(int minPrec = 1);
    NodePtr parsePrimary();

    int precedence(TokenType type) const;
    std::string rebuildHtmlString();
};

} // namespace UCLang
