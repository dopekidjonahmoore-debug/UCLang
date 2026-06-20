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
            std::to_string(peek().column) + " \u2014 " + hint);
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
        case TokenType::OP_OR:    return 1;
        case TokenType::OP_AND:   return 2;
        case TokenType::OP_EQUAL: case TokenType::OP_NEQ:
        case TokenType::OP_GTE:   case TokenType::OP_LTE:
        case TokenType::OP_GT:    case TokenType::OP_LT:
            return 3;
        case TokenType::OP_PLUS:  case TokenType::OP_MINUS:
            return 4;
        case TokenType::OP_STAR:  case TokenType::OP_SLASH:
            return 5;
        default: return 0;
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

    while (!atEnd() && peek().type != TokenType::END_OF_FILE) {
        try {
            auto stmt = parseStatement();
            prog.stmts.push_back(std::move(stmt));
        } catch (const std::runtime_error&) {
            while (!atEnd() && peek().type != TokenType::STMT_END) advance();
            if (peek().type == TokenType::STMT_END) advance();
        }
    }

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
            advance();
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

        // ── OOP statements ─────────────────────────────────
        case TokenType::KW_CLASS:
            return parseClassDef();

        case TokenType::KW_STRUCT:
            return parseStructDef();

        case TokenType::KW_INTERFACE:
            return parseInterfaceDef();

        case TokenType::KW_IMPORT:
            return parseImport();

        case TokenType::KW_YIELD:
            return parseYield();

        case TokenType::KW_NEW: {
            NodePtr expr = parseNewExpr();
            expect(TokenType::STMT_END, "expected . after new expression");
            return expr;
        }

        case TokenType::KW_THIS:
        case TokenType::KW_SUPER: {
            NodePtr expr = parsePrimary();
            // Check for member access or call
            if (peek().type == TokenType::OP_DOT || peek().type == TokenType::OP_ARROW) {
                expr = parsePostfixExpr(std::move(expr));
            }
            // Handle assignment to member: this.field == value
            if (auto* mg = std::get_if<MemberGetNode>(&expr->data)) {
                if (peek().type == TokenType::OP_ASSIGN) {
                    advance();
                    NodePtr val = parseExpr();
                    expect(TokenType::STMT_END, "expected . after member assignment");
                    auto ms = Node::make<MemberSetNode>();
                    auto& m = std::get<MemberSetNode>(ms->data);
                    m.object = std::move(mg->object);
                    m.member = mg->member;
                    m.value = std::move(val);
                    return ms;
                }
            }
            expect(TokenType::STMT_END, "expected . after expression");
            return expr;
        }
        case TokenType::KW_NULL: {
            NodePtr expr = parsePrimary();
            // Check for member access or call
            if (peek().type == TokenType::OP_DOT || peek().type == TokenType::OP_ARROW) {
                expr = parsePostfixExpr(std::move(expr));
            }
            expect(TokenType::STMT_END, "expected . after expression");
            return expr;
        }

        case TokenType::IDENTIFIER: {
            if (tok.value == "gameloop" && peek(1).type == TokenType::LPAREN)
                return parseGameLoop();

            // Check for variable assignment: var == expr
            if (peek(1).type == TokenType::OP_ASSIGN) {
                advance();
                return parseAssign(tok);
            }

            // Multi-assign: a, b == expr
            if (peek(1).type == TokenType::COMMA) {
                advance();
                return parseMultiAssign(tok);
            }

            // Type check: var = Int/Float/String/Bool
            if (peek(1).type == TokenType::OP_EQUAL) {
                TokenType t2 = peek(2).type;
                if (t2 == TokenType::TYPE_INT || t2 == TokenType::TYPE_FLOAT ||
                    t2 == TokenType::TYPE_STRING || t2 == TokenType::TYPE_BOOL) {
                    advance();
                    return parseTypeCheck(tok);
                }
            }

            // Parse as expression (may include member access chains)
            {
                NodePtr expr = parsePrimary();
                // Handle member access chains
                while (peek().type == TokenType::OP_DOT || peek().type == TokenType::OP_ARROW) {
                    expr = parsePostfixExpr(std::move(expr));
                }
                // Handle assignment to member: expr.id == value
                if (auto* mg = std::get_if<MemberGetNode>(&expr->data)) {
                    if (peek().type == TokenType::OP_ASSIGN) {
                        advance();
                        NodePtr val = parseExpr();
                        expect(TokenType::STMT_END, "expected . after member assignment");
                        auto ms = Node::make<MemberSetNode>();
                        auto& m = std::get<MemberSetNode>(ms->data);
                        m.object = std::move(mg->object);
                        m.member = mg->member;
                        m.value = std::move(val);
                        return ms;
                    }
                }
                expect(TokenType::STMT_END, "expected . after expression");
                return expr;
            }
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
//  Postfix expression: handle . and -> member access chains
// ==========================================================

NodePtr Parser::parsePostfixExpr(NodePtr obj) {
    while (peek().type == TokenType::OP_DOT || peek().type == TokenType::OP_ARROW) {
        advance(); // consume . or ->
        Token member = expect(TokenType::IDENTIFIER, "expected member name");

        if (peek().type == TokenType::LPAREN) {
            // Method call: obj.member(args.)
            advance(); // consume (
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

            auto mc = Node::make<MemberCallNode>();
            auto& m = std::get<MemberCallNode>(mc->data);
            m.object = std::move(obj);
            m.member = member.value;
            m.args = std::move(args);
            obj = std::move(mc);
        } else {
            // Property access: obj.member
            auto mg = Node::make<MemberGetNode>();
            auto& m = std::get<MemberGetNode>(mg->data);
            m.object = std::move(obj);
            m.member = member.value;
            obj = std::move(mg);
        }
    }
    return obj;
}

// ==========================================================
//  Function definition   def name(params. body. ).
// ==========================================================

NodePtr Parser::parseFuncDef() {
    Token nameTok = expect(TokenType::IDENTIFIER, "expected function name");
    expect(TokenType::LPAREN);

    std::vector<std::string> params;
    while (peek().type == TokenType::IDENTIFIER) {
        TokenType next = peek(1).type;
        if (next == TokenType::COMMA || next == TokenType::STMT_END || next == TokenType::LPAREN) {
            params.push_back(advance().value);
            if (peek().type == TokenType::COMMA) advance();
        } else break;
    }
    expect(TokenType::STMT_END, "expected . after parameter list");

    std::vector<NodePtr> body;
    while (!atEnd() && peek().type != TokenType::RPAREN) {
        try {
            body.push_back(parseStatement());
        } catch (const std::runtime_error&) {
            while (!atEnd() && peek().type != TokenType::STMT_END) advance();
            if (peek().type == TokenType::STMT_END) advance();
        }
    }

    expect(TokenType::RPAREN, "expected ) to close function definition");
    expect(TokenType::STMT_END, "expected . after function definition");

    return Node::make<FuncDefNode>(nameTok.value, std::move(params), std::move(body));
}

// ==========================================================
//  Function call   run.name(args.).
// ==========================================================

NodePtr Parser::parseFuncCall() {
    expect(TokenType::KW_RUN);
    if (peek().type == TokenType::OP_DOT || peek().type == TokenType::STMT_END)
        advance();
    else
        expect(TokenType::STMT_END);
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
        if (peek().type == TokenType::STMT_END) advance();
    }

    if (peek().type == TokenType::RPAREN) advance();
    expect(TokenType::STMT_END, "expected . after function call");

    return Node::make<FuncCallNode>(nameTok.value, std::move(args));
}

// ==========================================================
//  Function call in expression   name(args.)
// ==========================================================

NodePtr Parser::parseFuncCallExpr(const Token& nameTok) {
    advance();
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
//  Bare function-call statement   funcname(args.).
// ==========================================================

NodePtr Parser::parseFuncCallStmt(const Token& nameTok) {
    advance();
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
    expect(TokenType::STMT_END, "expected . after function call");

    return Node::make<FuncCallNode>(nameTok.value, std::move(args));
}

// ==========================================================
//  Namespace function call   Window.open(args.).
//  Still used for top-level namespace calls in expression
//  context (e.g. inside parsePrimary).
// ==========================================================

NodePtr Parser::parseNamespaceCall(const Token& ns, bool consumeStmtEnd) {
    advance();
    advance();
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
    advance();
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
    if (peek().type == TokenType::RPAREN) advance();
    if (peek().type == TokenType::STMT_END) advance();
    if (peek().type == TokenType::RPAREN) advance();
    if (peek().type == TokenType::STMT_END) advance();
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
            case TokenType::HTML_OPEN:    out << '<' << peek().value; advance(); break;
            case TokenType::HTML_CLOSE:   out << "</" << peek().value << '>'; advance(); break;
            case TokenType::HTML_SELF_CLOSE: out << " />"; advance(); break;
            case TokenType::HTML_TAG_END: out << '>'; advance(); break;
            case TokenType::HTML_TEXT:    out << peek().value; advance(); break;
            case TokenType::HTML_ATTR_NAME:
                out << ' ' << peek().value; advance();
                if (peek().type == TokenType::HTML_ATTR_VAL) {
                    out << "=\"" << peek().value << '"'; advance();
                }
                break;
            case TokenType::RPAREN: return out.str();
            default: return out.str();
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
    auto exprNode = Node::make<IdentifierNode>(ident.value);
    return Node::make<TypeCheckNode>(std::move(exprNode), typeTok.value);
}

// ==========================================================
//  ── OOP PARSING ──────────────────────────────────────────
// ==========================================================

// ==========================================================
//  Class definition
//  class ClassName extends Parent implements IFace1, IFace2 {
//      [public|private|protected] [static] [override] def method(...).
//      [public|private|protected] [static] var == expr.
//      [public|private|protected] [static] var.
//  }
// ==========================================================

NodePtr Parser::parseClassDef() {
    expect(TokenType::KW_CLASS);
    Token nameTok = expect(TokenType::IDENTIFIER, "expected class name");

    std::string parent;
    std::vector<std::string> interfaces;

    // extends
    if (peek().type == TokenType::KW_EXTENDS) {
        advance();
        parent = expect(TokenType::IDENTIFIER, "expected parent class name").value;
    }

    // implements
    if (peek().type == TokenType::KW_IMPLEMENTS) {
        advance();
        interfaces.push_back(expect(TokenType::IDENTIFIER, "expected interface name").value);
        while (peek().type == TokenType::COMMA) {
            advance();
            interfaces.push_back(expect(TokenType::IDENTIFIER, "expected interface name").value);
        }
    }

    // Body
    expect(TokenType::LBRACE, "expected { for class body");

    std::vector<ClassMemberNode> members;

    while (!atEnd() && peek().type != TokenType::RBRACE) {
        std::string visibility = "public";
        bool isStatic = false;
        bool isOverride = false;

        // Parse access modifiers
        while (true) {
            if (peek().type == TokenType::KW_PUBLIC) { visibility = "public"; advance(); }
            else if (peek().type == TokenType::KW_PRIVATE) { visibility = "private"; advance(); }
            else if (peek().type == TokenType::KW_PROTECTED) { visibility = "protected"; advance(); }
            else if (peek().type == TokenType::KW_STATIC) { isStatic = true; advance(); }
            else if (peek().type == TokenType::KW_OVERRIDE) { isOverride = true; advance(); }
            else break;
        }

        // Method definition
        if (peek().type == TokenType::KW_DEF) {
            advance();
            Token methodName = expect(TokenType::IDENTIFIER, "expected method name");
            expect(TokenType::LPAREN);

            std::vector<std::string> params;
            while (peek().type == TokenType::IDENTIFIER) {
                TokenType next = peek(1).type;
                if (next == TokenType::COMMA || next == TokenType::STMT_END || next == TokenType::LPAREN) {
                    params.push_back(advance().value);
                    if (peek().type == TokenType::COMMA) advance();
                } else break;
            }
            expect(TokenType::STMT_END, "expected . after method parameters");

            std::vector<NodePtr> body;
            while (!atEnd() && peek().type != TokenType::RPAREN) {
                try {
                    body.push_back(parseStatement());
                } catch (const std::runtime_error&) {
                    while (!atEnd() && peek().type != TokenType::STMT_END) advance();
                    if (peek().type == TokenType::STMT_END) advance();
                }
            }
            Token closeParen = advance();
            if (closeParen.type != TokenType::RPAREN)
                throw std::runtime_error("expected ) to close method");

            ClassMemberNode cm;
            cm.visibility = visibility;
            cm.isStatic = isStatic;
            cm.isOverride = isOverride;
            cm.name = methodName.value;
            cm.value = Node::make<FuncDefNode>(methodName.value, std::move(params), std::move(body));
            members.push_back(std::move(cm));
            continue;
        }

        // Field or property
        Token fieldTok = expect(TokenType::IDENTIFIER, "expected member name");

        if (peek().type == TokenType::OP_ASSIGN) {
            advance(); // ==
            NodePtr val = parseExpr();
            if (peek().type == TokenType::STMT_END) advance();

            ClassMemberNode cm;
            cm.visibility = visibility;
            cm.isStatic = isStatic;
            cm.isOverride = isOverride;
            cm.name = fieldTok.value;
            cm.value = std::move(val);
            members.push_back(std::move(cm));
        } else if (peek().type == TokenType::STMT_END) {
            advance(); // just a declaration without initializer
            ClassMemberNode cm;
            cm.visibility = visibility;
            cm.isStatic = isStatic;
            cm.isOverride = isOverride;
            cm.name = fieldTok.value;
            cm.value = nullptr;
            members.push_back(std::move(cm));
        } else if (peek().type == TokenType::LBRACE) {
            // Property with get/set
            // Skip for now - consume until RBRACE
            advance();
            while (!atEnd() && peek().type != TokenType::RBRACE) advance();
            if (peek().type == TokenType::RBRACE) advance();
        } else {
            throw std::runtime_error("unexpected token in class body");
        }
    }

    expect(TokenType::RBRACE, "expected } to close class");

    return Node::make<ClassDefNode>(nameTok.value, parent, std::move(interfaces), std::move(members));
}

// ==========================================================
//  Struct definition (simpler - no inheritance)
//  struct Name { ... }
// ==========================================================

NodePtr Parser::parseStructDef() {
    expect(TokenType::KW_STRUCT);
    Token nameTok = expect(TokenType::IDENTIFIER, "expected struct name");
    expect(TokenType::LBRACE, "expected { for struct body");

    std::vector<ClassMemberNode> members;

    while (!atEnd() && peek().type != TokenType::RBRACE) {
        std::string visibility = "public";
        bool isStatic = false;
        bool isOverride = false;

        while (true) {
            if (peek().type == TokenType::KW_PUBLIC) { visibility = "public"; advance(); }
            else if (peek().type == TokenType::KW_PRIVATE) { visibility = "private"; advance(); }
            else if (peek().type == TokenType::KW_PROTECTED) { visibility = "protected"; advance(); }
            else if (peek().type == TokenType::KW_STATIC) { isStatic = true; advance(); }
            else break;
        }

        if (peek().type == TokenType::KW_DEF) {
            advance();
            Token methodName = expect(TokenType::IDENTIFIER, "expected method name");
            expect(TokenType::LPAREN);
            std::vector<std::string> params;
            while (peek().type == TokenType::IDENTIFIER) {
                TokenType next = peek(1).type;
                if (next == TokenType::COMMA || next == TokenType::STMT_END || next == TokenType::LPAREN) {
                    params.push_back(advance().value);
                    if (peek().type == TokenType::COMMA) advance();
                } else break;
            }
            expect(TokenType::STMT_END, "expected . after method params");
            std::vector<NodePtr> body;
            while (!atEnd() && peek().type != TokenType::RPAREN) {
                try { body.push_back(parseStatement()); }
                catch (const std::runtime_error&) {
                    while (!atEnd() && peek().type != TokenType::STMT_END) advance();
                    if (peek().type == TokenType::STMT_END) advance();
                }
            }
            expect(TokenType::RPAREN, "expected )");
            ClassMemberNode cm;
            cm.visibility = visibility; cm.isStatic = isStatic; cm.isOverride = isOverride;
            cm.name = methodName.value;
            cm.value = Node::make<FuncDefNode>(methodName.value, std::move(params), std::move(body));
            members.push_back(std::move(cm));
        } else {
            Token fieldTok = expect(TokenType::IDENTIFIER, "expected field name");
            if (peek().type == TokenType::OP_ASSIGN) {
                advance(); NodePtr val = parseExpr();
                if (peek().type == TokenType::STMT_END) advance();
                ClassMemberNode cm;
                cm.visibility = visibility; cm.isStatic = isStatic; cm.isOverride = false;
                cm.name = fieldTok.value; cm.value = std::move(val);
                members.push_back(std::move(cm));
            } else if (peek().type == TokenType::STMT_END) {
                advance();
                ClassMemberNode cm;
                cm.visibility = visibility; cm.isStatic = isStatic; cm.isOverride = false;
                cm.name = fieldTok.value; cm.value = nullptr;
                members.push_back(std::move(cm));
            } else throw std::runtime_error("unexpected token in struct body");
        }
    }

    expect(TokenType::RBRACE, "expected } to close struct");
    return Node::make<ClassDefNode>(nameTok.value, "", std::vector<std::string>(), std::move(members));
}

// ==========================================================
//  Interface definition  interface Name { def meth(). }
// ==========================================================

NodePtr Parser::parseInterfaceDef() {
    expect(TokenType::KW_INTERFACE);
    Token nameTok = expect(TokenType::IDENTIFIER, "expected interface name");

    if (peek().type == TokenType::KW_EXTENDS) {
        advance();
        while (peek().type == TokenType::IDENTIFIER) advance(); // skip parent interfaces
    }

    expect(TokenType::LBRACE, "expected { for interface body");

    std::vector<ClassMemberNode> members;

    while (!atEnd() && peek().type != TokenType::RBRACE) {
        if (peek().type == TokenType::KW_DEF) {
            advance();
            Token methodName = expect(TokenType::IDENTIFIER, "expected method name");
            expect(TokenType::LPAREN);
            std::vector<std::string> params;
            while (peek().type == TokenType::IDENTIFIER) {
                TokenType next = peek(1).type;
                if (next == TokenType::COMMA || next == TokenType::STMT_END || next == TokenType::LPAREN) {
                    params.push_back(advance().value);
                    if (peek().type == TokenType::COMMA) advance();
                } else break;
            }
            if (peek().type == TokenType::STMT_END) advance();
            if (peek().type == TokenType::RPAREN) advance();

            auto emptyBody = std::vector<NodePtr>();
            ClassMemberNode cm;
            cm.visibility = "public"; cm.isStatic = false; cm.isOverride = false;
            cm.name = methodName.value;
            cm.value = Node::make<FuncDefNode>(methodName.value, std::move(params), std::move(emptyBody));
            members.push_back(std::move(cm));
        } else {
            advance(); // skip unknown tokens
        }
    }

    expect(TokenType::RBRACE, "expected } to close interface");
    return Node::make<ClassDefNode>(nameTok.value, "", std::vector<std::string>(), std::move(members));
}

// ==========================================================
//  new expression  new ClassName(args.)
// ==========================================================

NodePtr Parser::parseNewExpr() {
    expect(TokenType::KW_NEW);
    Token className = expect(TokenType::IDENTIFIER, "expected class name after new");
    expect(TokenType::LPAREN, "expected ( after class name");

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

    return Node::make<NewExprNode>(className.value, std::move(args));
}

// ==========================================================
//  yield  yield(expr.)
// ==========================================================

NodePtr Parser::parseYield() {
    expect(TokenType::KW_YIELD);
    expect(TokenType::LPAREN);
    NodePtr val = parseExpr();
    if (peek().type == TokenType::STMT_END) advance();
    expect(TokenType::RPAREN, "expected ) after yield expression");
    expect(TokenType::STMT_END, "expected . after yield");
    return Node::make<YieldNode>(std::move(val));
}

// ==========================================================
//  import  import "path"
// ==========================================================

NodePtr Parser::parseImport() {
    expect(TokenType::KW_IMPORT);
    Token pathTok = expect(TokenType::LIT_STRING, "expected string path after import");
    expect(TokenType::STMT_END, "expected . after import");
    return Node::make<ImportNode>(pathTok.value);
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

        // Handle member access: expr.id or expr.id(args.)
        if (peek().type == TokenType::OP_DOT || peek().type == TokenType::OP_ARROW) {
            left = parsePostfixExpr(std::move(left));
            continue;
        }

        // Handle 'as' type cast: expr as Type
        if (peek().type == TokenType::KW_AS) {
            advance();
            Token typeTok = expect(TokenType::IDENTIFIER, "expected type name after as");
            auto ac = Node::make<AsCastNode>();
            auto& a = std::get<AsCastNode>(ac->data);
            a.expr = std::move(left);
            a.typeName = typeTok.value;
            left = std::move(ac);
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

        case TokenType::KW_NULL: {
            advance();
            return Node::make<NullNode>();
        }

        case TokenType::KW_THIS: {
            advance();
            return Node::make<ThisExprNode>();
        }

        case TokenType::KW_SUPER: {
            advance();
            return Node::make<SuperExprNode>();
        }

        case TokenType::KW_NEW: {
            return parseNewExpr();
        }

        case TokenType::IDENTIFIER:
        case TokenType::BUILTIN_INPUT: {
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
            advance();
            if (peek().type == TokenType::OP_DOT || peek().type == TokenType::STMT_END)
                advance();
            else expect(TokenType::STMT_END);
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
                while (peek().type == TokenType::COMMA) { advance(); args.push_back(parseExpr()); }
            }
            if (peek().type == TokenType::STMT_END) advance();
            if (peek().type == TokenType::RPAREN) advance();
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
