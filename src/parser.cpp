#include "parser.h"
#include <cstdio>
#include <stdexcept>
#include <sstream>

namespace UCLang {

// ==========================================================
//  Helpers
// ==========================================================

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
{
    m_tokens.shrink_to_fit();
}

const Token& Parser::peek(int offset) const {
    size_t idx = m_pos + static_cast<size_t>(offset);
    if (idx >= m_tokens.size())
        return m_tokens.back();
    return m_tokens[idx];
}

Token Parser::advance() {
    if (atEnd()) return m_tokens.back();
    return m_tokens[m_pos++];
}

bool Parser::atEnd() const {
    return m_pos >= m_tokens.size()
        || m_tokens[m_pos].type == TokenType::END_OF_FILE;
}

Token Parser::expect(TokenType type, const std::string& msg) {
    if (atEnd() || peek().type != type) {
        std::string hint = msg.empty()
            ? std::string("expected ") + tokenTypeName(type)
            : msg;
        throw std::runtime_error("Parse error at Ln " +
            std::to_string(peek().line) + ":" +
            std::to_string(peek().column) + " — " + hint);
    }
    return advance();
}

bool Parser::match(TokenType type) {
    if (!atEnd() && peek().type == type) {
        advance();
        return true;
    }
    return false;
}

Token Parser::previous() const {
    if (m_pos == 0) return m_tokens[0];
    return m_tokens[m_pos - 1];
}

// ==========================================================
//  Precedence table
// ==========================================================

int Parser::precedence(TokenType type) const {
    switch (type) {
        case TokenType::OP_OR:
            return 1;
        case TokenType::OP_AND:
            return 2;
        case TokenType::OP_EQUAL:
        case TokenType::OP_NEQ:
        case TokenType::OP_GTE:
        case TokenType::OP_LTE:
        case TokenType::OP_GT:
        case TokenType::OP_LT:
            return 3;
        case TokenType::OP_PLUS:
        case TokenType::OP_MINUS:
            return 4;
        case TokenType::OP_STAR:
        case TokenType::OP_SLASH:
            return 5;
        default:
            return 0;
    }
}

// ==========================================================
//  Entry
// ==========================================================

NodePtr Parser::parse() {
    return parseProgram();
}

NodePtr Parser::parseProgram() {
    auto node = Node::make<ProgramNode>();
    auto& prog = std::get<ProgramNode>(node->data);
    int stmt_count = 0;

    while (!atEnd() && peek().type != TokenType::END_OF_FILE) {
        try {
            auto stmt = parseStatement();
            if (std::get_if<FuncDefNode>(&stmt->data)) {
                auto& fd = std::get<FuncDefNode>(stmt->data);
                printf("  PUSH FUNC %s (total %d)\n", fd.name.c_str(), stmt_count + 1);
            }
            prog.stmts.push_back(std::move(stmt));
            stmt_count++;
        } catch (const std::runtime_error&) {
            printf("  PARSE ERROR at Ln %d, sync to next .\n", peek().line);
            while (!atEnd() && peek().type != TokenType::STMT_END) advance();
            if (peek().type == TokenType::STMT_END) advance();
        }
    }
    printf("  TOTAL STMTS PARSED: %d\n", stmt_count);

    return node;
}

// ==========================================================
//  Statement dispatch
// ==========================================================

NodePtr Parser::parseStatement() {
    while (peek().type == TokenType::STMT_END) advance();
    if (atEnd()) throw std::runtime_error("Unexpected end of file");

    Token tok = peek();

    switch (tok.type) {

        case TokenType::KW_DEF: {
            advance(); // consume 'def'
            return parseFuncDef();
        }

        case TokenType::KW_RUN:
            return parseFuncCall();

        case TokenType::KW_IF:
            return parseIf();

        case TokenType::BUILTIN_PRINT:
            return parsePrint();

        case TokenType::BUILTIN_INPUT:
            if (peek(1).type == TokenType::OP_DOT)
                return parseNamespaceCall(tok, true);
            return parseInput();

        case TokenType::BUILTIN_HTML:
            return parseHtml();

        case TokenType::KW_WHILE:
            return parseWhile();

        case TokenType::KW_BREAK:
            return parseBreak();

        case TokenType::KW_NEXT:
            return parseNext();

        case TokenType::KW_RESPONSE:
            return parseReturn();

        case TokenType::IDENTIFIER: {
            if (peek(1).type == TokenType::OP_DOT)
                return parseNamespaceCall(tok, true);
            if (tok.value == "gameloop" && peek(1).type == TokenType::LPAREN)
                return parseGameLoop();
            if (peek(1).type == TokenType::OP_ASSIGN) {
                advance();
                return parseAssign(tok);
            }
    if (peek(1).type == TokenType::COMMA) {
        advance();
        return parseMultiAssign(tok);
    }
            if (peek(1).type == TokenType::OP_EQUAL) {
                TokenType t2 = peek(2).type;
                if (t2 == TokenType::TYPE_INT   ||
                    t2 == TokenType::TYPE_FLOAT ||
                    t2 == TokenType::TYPE_STRING ||
                    t2 == TokenType::TYPE_BOOL) {
                    advance();
                    return parseTypeCheck(tok);
                }
            }
            if (peek(1).type == TokenType::LPAREN)
                return parseFuncDef();
            break;
        }

        default:
            break;
    }

    // Expression statement
    NodePtr expr = parseExpr();
    expect(TokenType::STMT_END, "expected . after expression");
    return expr;
}

// ==========================================================
//  Function definition   funcName(params. body. ).
// ==========================================================

NodePtr Parser::parseFuncDef() {
    Token nameTok = expect(TokenType::IDENTIFIER, "expected function name");
    printf("  PARSING FUNC: %s\n", nameTok.value.c_str());
    expect(TokenType::LPAREN);

    // Parse params until DOT or RPAREN.
    // An identifier is a param only if followed by `,`, `.`, or `(`.
    // Otherwise (e.g. followed by `==`) it's a body statement.
    std::vector<std::string> params;
    while (peek().type == TokenType::IDENTIFIER) {
        TokenType next = peek(1).type;
        if (next == TokenType::COMMA || next == TokenType::STMT_END || next == TokenType::LPAREN) {
            params.push_back(advance().value);
            if (peek().type == TokenType::COMMA) advance();
        } else break;
    }
    expect(TokenType::STMT_END, "expected . after parameter list");

    // Parse body statements until closing RPAREN
    std::vector<NodePtr> body;
    while (!atEnd() && peek().type != TokenType::RPAREN) {
        try {
            body.push_back(parseStatement());
        } catch (const std::runtime_error& e) {
            // Skip failed statement; sync to next STMT_END
            while (!atEnd() && peek().type != TokenType::STMT_END) advance();
            if (peek().type == TokenType::STMT_END) advance();
        }
    }

    printf("  FUNC '%s' body done (stmts: %zu), closing ", nameTok.value.c_str(), body.size());

    expect(TokenType::RPAREN, "expected ) to close function definition");
    expect(TokenType::STMT_END, "expected . after function definition");
    printf("OK\n");

    return Node::make<FuncDefNode>(nameTok.value, std::move(params), std::move(body));
}

// ==========================================================
//  Function call   run.name(args.).
// ==========================================================

NodePtr Parser::parseFuncCall() {
    expect(TokenType::KW_RUN);
    // Accept either STMT_END (default) or OP_DOT for run.func()
    if (peek().type == TokenType::OP_DOT || peek().type == TokenType::STMT_END)
        advance();
    else
        expect(TokenType::STMT_END);          // the . after "run"
    Token nameTok = advance();
    if (nameTok.type != TokenType::IDENTIFIER
        && nameTok.type != TokenType::BUILTIN_PRINT
        && nameTok.type != TokenType::BUILTIN_INPUT
        && nameTok.type != TokenType::BUILTIN_HTML)
        throw std::runtime_error("expected function name after run.");
    expect(TokenType::LPAREN);

    std::vector<NodePtr> args;
    if (peek().type != TokenType::STMT_END && peek().type != TokenType::RPAREN) {
        args.push_back(parseExpr());
        while (peek().type == TokenType::COMMA) {
            advance();
            args.push_back(parseExpr());
        }
    } else {
        // No args: consume the DOT arg terminator if present
        if (peek().type == TokenType::STMT_END) advance();
    }

    if (peek().type == TokenType::RPAREN) advance();
    expect(TokenType::STMT_END, "expected . after function call");

    return Node::make<FuncCallNode>(nameTok.value, std::move(args));
}

// ==========================================================
//  Function call in expression   name(args.).
//  Used for calls like int_to_str(...) inside expressions.
// ==========================================================

NodePtr Parser::parseFuncCallExpr(const Token& nameTok) {
    advance(); // consume identifier (nameTok)
    expect(TokenType::LPAREN);

    std::vector<NodePtr> args;
    if (peek().type != TokenType::STMT_END && peek().type != TokenType::RPAREN) {
        args.push_back(parseExpr());
        while (peek().type == TokenType::COMMA) {
            advance();
            args.push_back(parseExpr());
        }
    }
    if (peek().type == TokenType::STMT_END) advance();
    if (peek().type == TokenType::RPAREN) advance();

    return Node::make<FuncCallNode>(nameTok.value, std::move(args));
}

// ==========================================================
//  Namespace function call   Window.open(args.).
// ==========================================================

NodePtr Parser::parseNamespaceCall(const Token& ns, bool consumeStmtEnd) {
    advance(); // consume namespace identifier
    advance(); // consume OP_DOT
    Token nameTok = expect(TokenType::IDENTIFIER, "expected function name after namespace.");
    expect(TokenType::LPAREN, "expected ( after function name");

    std::vector<NodePtr> args;
    if (peek().type != TokenType::STMT_END && peek().type != TokenType::RPAREN) {
        args.push_back(parseExpr());
        while (peek().type == TokenType::COMMA) {
            advance();
            args.push_back(parseExpr());
        }
    }
    if (peek().type == TokenType::STMT_END) advance();
    if (peek().type == TokenType::RPAREN) advance();
    if (consumeStmtEnd) expect(TokenType::STMT_END, "expected . after function call");

    return Node::make<FuncCallNode>(ns.value + "." + nameTok.value, std::move(args));
}

// ==========================================================
//  GameLoop   GameLoop(update(body.). draw(body.).).
// ==========================================================

NodePtr Parser::parseGameLoop() {
    advance(); // consume "GameLoop" identifier
    expect(TokenType::LPAREN);

    expect(TokenType::IDENTIFIER, "expected 'update' section");
    if (previous().value != "update")
        throw std::runtime_error("expected 'update' section in GameLoop");
    expect(TokenType::LPAREN);
    std::vector<NodePtr> updateBody;
    while (!atEnd() && peek().type != TokenType::RPAREN)
        updateBody.push_back(parseStatement());
    if (peek().type == TokenType::RPAREN) advance();
    if (peek().type == TokenType::STMT_END) advance();

    expect(TokenType::IDENTIFIER, "expected 'draw' section");
    if (previous().value != "draw")
        throw std::runtime_error("expected 'draw' section in GameLoop");
    expect(TokenType::LPAREN);
    std::vector<NodePtr> drawBody;
    while (!atEnd() && peek().type != TokenType::RPAREN)
        drawBody.push_back(parseStatement());
    if (peek().type == TokenType::RPAREN) advance();
    if (peek().type == TokenType::STMT_END) advance();

    if (peek().type == TokenType::RPAREN) advance();
    if (peek().type == TokenType::STMT_END) advance();

    return Node::make<GameLoopNode>(std::move(updateBody), std::move(drawBody));
}

// ==========================================================
//  If   If(condition then(body.). else(body.).).
// ==========================================================

NodePtr Parser::parseIf() {
    expect(TokenType::KW_IF);
    expect(TokenType::LPAREN);

    NodePtr cond = parseExpr();

    expect(TokenType::KW_THEN);
    expect(TokenType::LPAREN);

    std::vector<NodePtr> thenBody;
    while (!atEnd() && peek().type != TokenType::RPAREN && peek().type != TokenType::KW_ELSE)
        thenBody.push_back(parseStatement());
    if (peek().type == TokenType::RPAREN) { advance(); }
    if (peek().type == TokenType::STMT_END) { advance(); }

    std::vector<NodePtr> elseBody;
    if (peek().type == TokenType::KW_ELSE) {
        advance();
        expect(TokenType::LPAREN);
        while (!atEnd() && peek().type != TokenType::RPAREN)
            elseBody.push_back(parseStatement());
        if (peek().type == TokenType::RPAREN) { advance(); }
        if (peek().type == TokenType::STMT_END) { advance(); }
    }

    if (peek().type == TokenType::RPAREN) advance();
    if (peek().type == TokenType::STMT_END) advance();

    return Node::make<IfNode>(std::move(cond), std::move(thenBody), std::move(elseBody));
}

// ==========================================================
//  While   while(cond then(body.).).
// ==========================================================

NodePtr Parser::parseWhile() {
    expect(TokenType::KW_WHILE);
    expect(TokenType::LPAREN);
    NodePtr cond = parseExpr();
    expect(TokenType::KW_THEN);
    expect(TokenType::LPAREN);
    std::vector<NodePtr> body;
    while (!atEnd() && peek().type != TokenType::RPAREN)
        body.push_back(parseStatement());
    if (peek().type == TokenType::RPAREN) advance();      // ) close then body
    if (peek().type == TokenType::STMT_END) advance();    // .
    if (peek().type == TokenType::RPAREN) advance();      // ) close While(
    if (peek().type == TokenType::STMT_END) advance();    // .
    return Node::make<WhileNode>(std::move(cond), std::move(body));
}

// ==========================================================
//  Break   break(.).
// ==========================================================

NodePtr Parser::parseBreak() {
    expect(TokenType::KW_BREAK);
    expect(TokenType::LPAREN);
    if (peek().type == TokenType::STMT_END) advance();
    expect(TokenType::RPAREN);
    expect(TokenType::STMT_END);
    return Node::make<BreakNode>();
}

// ==========================================================
//  Next    next(.).
// ==========================================================

NodePtr Parser::parseNext() {
    expect(TokenType::KW_NEXT);
    expect(TokenType::LPAREN);
    if (peek().type == TokenType::STMT_END) advance();
    expect(TokenType::RPAREN);
    expect(TokenType::STMT_END);
    return Node::make<NextNode>();
}

// ==========================================================
//  Print   Print(expr.).
// ==========================================================

NodePtr Parser::parsePrint() {
    expect(TokenType::BUILTIN_PRINT);
    expect(TokenType::LPAREN);
    NodePtr val = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::STMT_END);
    return Node::make<PrintNode>(std::move(val));
}

// ==========================================================
//  Input   Input(prompt? response==var.).
// ==========================================================

NodePtr Parser::parseInput() {
    expect(TokenType::BUILTIN_INPUT);
    expect(TokenType::LPAREN);
    NodePtr prompt = parseExpr();
    expect(TokenType::KW_RESPONSE);
    expect(TokenType::OP_ASSIGN);
    Token varTok = expect(TokenType::IDENTIFIER, "expected variable name in Input");
    expect(TokenType::RPAREN);
    expect(TokenType::STMT_END);
    return Node::make<InputNode>(std::move(prompt), varTok.value);
}

// ==========================================================
//  Html   Html(<html>...</html>.).
//  Reconstructs raw HTML string from HTML tokens emitted by the lexer.
// ==========================================================

NodePtr Parser::parseHtml() {
    expect(TokenType::BUILTIN_HTML);
    expect(TokenType::LPAREN);
    std::string raw = rebuildHtmlString();
    expect(TokenType::RPAREN);
    expect(TokenType::STMT_END);
    return Node::make<HtmlNode>(raw);
}

std::string Parser::rebuildHtmlString() {
    std::ostringstream out;
    while (!atEnd()) {
        switch (peek().type) {
            case TokenType::HTML_OPEN:
                out << '<' << peek().value;
                advance();
                break;
            case TokenType::HTML_CLOSE:
                out << "</" << peek().value << '>';
                advance();
                break;
            case TokenType::HTML_SELF_CLOSE:
                out << " />";
                advance();
                break;
            case TokenType::HTML_TAG_END:
                out << '>';
                advance();
                break;
            case TokenType::HTML_TEXT:
                out << peek().value;
                advance();
                break;
            case TokenType::HTML_ATTR_NAME:
                out << ' ' << peek().value;
                advance();
                if (peek().type == TokenType::HTML_ATTR_VAL) {
                    out << "=\"" << peek().value << '"';
                    advance();
                }
                break;
            case TokenType::RPAREN:
                return out.str();
            default:
                return out.str();
        }
    }
    return out.str();
}

// ==========================================================
//  Return   response(expr.).
// ==========================================================

NodePtr Parser::parseReturn() {
    expect(TokenType::KW_RESPONSE);
    expect(TokenType::LPAREN);

    // No-arg return: response(.).
    if (peek().type == TokenType::STMT_END) {
        advance();
        expect(TokenType::RPAREN, "expected ) to close response()");
        expect(TokenType::STMT_END, "expected . after response()");
        std::vector<NodePtr> empty;
        return Node::make<ReturnNode>(Node::make<ListLiteralNode>(std::move(empty)));
    }

    std::vector<NodePtr> values;
    values.push_back(parseExpr());
    while (peek().type == TokenType::COMMA) {
        advance();
        values.push_back(parseExpr());
    }
    expect(TokenType::RPAREN, "expected ) to close response()");
    expect(TokenType::STMT_END, "expected . after response()");
    if (values.size() == 1)
        return Node::make<ReturnNode>(std::move(values[0]));
    return Node::make<ReturnNode>(Node::make<ListLiteralNode>(std::move(values)));
}

// ==========================================================
//  Assignment   var == expr.
// ==========================================================

NodePtr Parser::parseAssign(const Token& ident) {
    expect(TokenType::OP_ASSIGN);
    NodePtr val = parseExpr();
    expect(TokenType::STMT_END);
    return Node::make<AssignNode>(ident.value, std::move(val));
}

// ==========================================================
//  Multi-assign   a, b == expr.
// ==========================================================

NodePtr Parser::parseMultiAssign(const Token& ident) {
    std::vector<std::string> vars;
    vars.push_back(ident.value);
    while (peek().type == TokenType::COMMA) {
        advance();
        vars.push_back(expect(TokenType::IDENTIFIER, "expected variable name").value);
    }
    expect(TokenType::OP_ASSIGN, "expected == after variables");
    NodePtr val = parseExpr();
    expect(TokenType::STMT_END);
    return Node::make<MultiAssignNode>(std::move(vars), std::move(val));
}

// ==========================================================
//  Type check   expr = Int / Float / String / Bool
// ==========================================================

NodePtr Parser::parseTypeCheck(const Token& ident) {
    expect(TokenType::OP_EQUAL);
    Token typeTok = advance();
    expect(TokenType::STMT_END);
    // Build an Identifier node for the variable being checked
    auto exprNode = Node::make<IdentifierNode>(ident.value);
    return Node::make<TypeCheckNode>(std::move(exprNode), typeTok.value);
}

// ==========================================================
//  Expressions (precedence climbing)
// ==========================================================

NodePtr Parser::parseExpr() {
    return parseBinaryOp(1);
}

NodePtr Parser::parseBinaryOp(int minPrec) {
    NodePtr left = parsePrimary();

    while (!atEnd()) {
        // Handle list indexing: expr[expr]
        if (peek().type == TokenType::LBRACKET) {
            advance();
            NodePtr idx = parseExpr();
            expect(TokenType::RBRACKET, "expected ] after index");
            auto get = Node::make<ListGetNode>();
            auto& g = std::get<ListGetNode>(get->data);
            g.list = std::move(left);
            g.index = std::move(idx);
            left = std::move(get);
            continue;
        }

        TokenType opType = peek().type;
        int prec = precedence(opType);
        if (prec == 0 || prec < minPrec) break;

        advance();
        std::string opStr = previous().value;

        NodePtr right = parseBinaryOp(prec + 1);

        auto bin = Node::make<BinaryOpNode>();
        auto& b = std::get<BinaryOpNode>(bin->data);
        b.op = opStr;
        b.left = std::move(left);
        b.right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

NodePtr Parser::parsePrimary() {
    if (atEnd()) throw std::runtime_error("Unexpected end of expression");

    Token tok = peek();

    switch (tok.type) {

        case TokenType::LIT_STRING: {
            advance();
            return Node::make<LiteralNode>(std::string(tok.value));
        }

        case TokenType::LIT_INT: {
            advance();
            return Node::make<LiteralNode>(static_cast<int64_t>(std::stoll(tok.value)));
        }

        case TokenType::LIT_FLOAT: {
            advance();
            return Node::make<LiteralNode>(std::stod(tok.value));
        }

        case TokenType::LIT_BOOL: {
            advance();
            return Node::make<LiteralNode>(tok.value == "true");
        }

        case TokenType::IDENTIFIER:
        case TokenType::BUILTIN_INPUT: {
            if (peek(1).type == TokenType::OP_DOT)
                return parseNamespaceCall(tok, false);
            if (tok.type == TokenType::IDENTIFIER && peek(1).type == TokenType::LPAREN)
                return parseFuncCallExpr(tok);
            advance();
            return Node::make<IdentifierNode>(tok.value);
        }

        case TokenType::LBRACKET: {
            advance();
            std::vector<NodePtr> elems;
            if (peek().type != TokenType::RBRACKET) {
                elems.push_back(parseExpr());
                while (peek().type == TokenType::COMMA) {
                    advance();
                    elems.push_back(parseExpr());
                }
            }
            expect(TokenType::RBRACKET, "expected ] after list literal");
            return Node::make<ListLiteralNode>(std::move(elems));
        }

        case TokenType::LPAREN: {
            advance();
            NodePtr expr = parseExpr();
            expect(TokenType::RPAREN, "expected ) after sub-expression");
            return expr;
        }

        case TokenType::OP_MINUS: {
            advance();
            NodePtr val = parsePrimary();
            auto bin = Node::make<BinaryOpNode>();
            auto& b = std::get<BinaryOpNode>(bin->data);
            b.op = "-";
            b.left = Node::make<LiteralNode>(static_cast<int64_t>(0));
            b.right = std::move(val);
            return bin;
        }

        case TokenType::KW_RUN: {
            // Function call inside an expression: run.name(args.).
            // Consume run, ., name, (, args., ) but NOT the trailing STMT_END.
            advance(); // run
            if (peek().type == TokenType::OP_DOT || peek().type == TokenType::STMT_END)
                advance(); // .
            else
                expect(TokenType::STMT_END); // .
            Token nameTok = advance();
            if (nameTok.type != TokenType::IDENTIFIER
                && nameTok.type != TokenType::BUILTIN_PRINT
                && nameTok.type != TokenType::BUILTIN_INPUT
                && nameTok.type != TokenType::BUILTIN_HTML)
                throw std::runtime_error("expected function name after run.");
            expect(TokenType::LPAREN); // (
            std::vector<NodePtr> args;
            if (peek().type != TokenType::STMT_END && peek().type != TokenType::RPAREN) {
                args.push_back(parseExpr());
                while (peek().type == TokenType::COMMA) { advance(); args.push_back(parseExpr()); }
            }
            if (peek().type == TokenType::STMT_END) advance(); // consume arg/no-arg terminator .
            if (peek().type == TokenType::RPAREN) advance();    // consume )
            return Node::make<FuncCallNode>(nameTok.value, std::move(args));
        }

        default:
            throw std::runtime_error(
                "Unexpected token " + std::string(tokenTypeName(tok.type)) +
                " (" + tok.value + ") at Ln " +
                std::to_string(tok.line) + ":" +
                std::to_string(tok.column));
    }
}

} // namespace UCLang
