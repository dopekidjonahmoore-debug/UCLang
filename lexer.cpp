#include "lexer.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace UCLang {



// ─────────────────────────────────────────────────────────────────
//  Keyword table
//  Maps raw identifier text → TokenType for every reserved word.
// ─────────────────────────────────────────────────────────────────
const std::unordered_map<std::string, TokenType> Lexer::s_keywords = {
    { "def",      TokenType::KW_DEF      },
    { "run",      TokenType::KW_RUN      },
    { "if",       TokenType::KW_IF       },
    { "then",     TokenType::KW_THEN     },
    { "else",     TokenType::KW_ELSE     },
    { "response", TokenType::KW_RESPONSE },
    { "while",    TokenType::KW_WHILE    },
    { "break",    TokenType::KW_BREAK    },
    { "next",     TokenType::KW_NEXT     },
    { "true",     TokenType::LIT_BOOL    },
    { "false",    TokenType::LIT_BOOL    },
    { "int",      TokenType::TYPE_INT    },
    { "float",    TokenType::TYPE_FLOAT  },
    { "string",   TokenType::TYPE_STRING },
    { "bool",     TokenType::TYPE_BOOL   },
    { "print",    TokenType::BUILTIN_PRINT },
    { "input",    TokenType::BUILTIN_INPUT },
    { "html",     TokenType::BUILTIN_HTML  },
    { "and",      TokenType::OP_AND       },
    { "or",       TokenType::OP_OR        },
    // ── OOP keywords ─────────────────────────────────────────────
    { "class",      TokenType::KW_CLASS      },
    { "struct",     TokenType::KW_STRUCT     },
    { "interface",  TokenType::KW_INTERFACE  },
    { "extends",    TokenType::KW_EXTENDS    },
    { "implements", TokenType::KW_IMPLEMENTS },
    { "new",        TokenType::KW_NEW        },
    { "this",       TokenType::KW_THIS       },
    { "super",      TokenType::KW_SUPER      },
    { "public",     TokenType::KW_PUBLIC     },
    { "private",    TokenType::KW_PRIVATE    },
    { "protected",  TokenType::KW_PROTECTED  },
    { "override",   TokenType::KW_OVERRIDE   },
    { "virtual",    TokenType::KW_VIRTUAL    },
    { "abstract",   TokenType::KW_ABSTRACT   },
    { "static",     TokenType::KW_STATIC     },
    { "const",      TokenType::KW_CONST      },
    { "yield",      TokenType::KW_YIELD      },
    { "as",         TokenType::KW_AS         },
    { "ref",        TokenType::KW_REF        },
    { "var",        TokenType::KW_VAR        },
    { "null",       TokenType::KW_NULL       },
    { "import",     TokenType::KW_IMPORT     },
    { "namespace",  TokenType::KW_NAMESPACE  },
    { "get",        TokenType::KW_GET        },
    { "set",        TokenType::KW_SET        },
};

// ─────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────
Lexer::Lexer(std::string source) : m_source(std::move(source)) {}

// ─────────────────────────────────────────────────────────────────
//  Low-level helpers
// ─────────────────────────────────────────────────────────────────
char Lexer::peek(int offset) const {
    size_t idx = m_pos + static_cast<size_t>(offset);
    return (idx < m_source.size()) ? m_source[idx] : '\0';
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') { ++m_line; m_col = 1; }
    else           { ++m_col; }
    return c;
}

bool Lexer::atEnd() const { return m_pos >= m_source.size(); }

void Lexer::skipWhitespace() {
    while (!atEnd() && std::isspace(static_cast<unsigned char>(peek())))
        advance();
}

// Returns true and consumes the comment if we're sitting on ##
bool Lexer::skipBlockComment() {
    if (peek(0) != '#' || peek(1) != '#') return false;
    advance(); advance();  // consume ##
    while (!atEnd()) {
        if (peek(0) == '#' && peek(1) == '#') {
            advance(); advance();  // consume ##
            return true;
        }
        advance();
    }
    throw std::runtime_error(
        "Unterminated block comment — missing closing ##  (opened near line "
        + std::to_string(m_line) + ")");
}

// ─────────────────────────────────────────────────────────────────
//  String scanner    "hello\nworld"
// ─────────────────────────────────────────────────────────────────
Token Lexer::scanString() {
    int sl = m_line, sc = m_col;
    advance();  // consume opening "
    std::string val;

    while (!atEnd() && peek() != '"') {
        if (peek() == '\\') {
            advance();  // consume backslash
            switch (advance()) {
                case 'n':  val += '\n'; break;
                case 't':  val += '\t'; break;
                case 'r':  val += '\r'; break;
                case '"':  val += '"';  break;
                case '\\': val += '\\'; break;
                default:   val += '\\'; // preserve unknown escapes
            }
        } else {
            val += advance();
        }
    }

    if (atEnd())
        throw std::runtime_error("Unterminated string literal at line "
                                 + std::to_string(sl) + ":" + std::to_string(sc));
    advance();  // consume closing "
    return { TokenType::LIT_STRING, val, sl, sc };
}

// ─────────────────────────────────────────────────────────────────
//  Number scanner    42  /  3.14
//  The decimal '.' is ONLY consumed as part of a float when the
//  next character is a digit, otherwise it stays as a STMT_END.
// ─────────────────────────────────────────────────────────────────
Token Lexer::scanNumber() {
    int sl = m_line, sc = m_col;
    std::string val;
    bool isFloat = false;

    while (!atEnd()) {
        char c = peek();
        if (std::isdigit(static_cast<unsigned char>(c))) {
            val += advance();
        } else if (c == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
            isFloat = true;
            val += advance();  // consume '.'
        } else {
            break;
        }
    }

    return { isFloat ? TokenType::LIT_FLOAT : TokenType::LIT_INT, val, sl, sc };
}

// ─────────────────────────────────────────────────────────────────
//  Identifier / keyword scanner
// ─────────────────────────────────────────────────────────────────
Token Lexer::scanIdentifierOrKeyword() {
    int sl = m_line, sc = m_col;
    std::string raw;

    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
        raw += advance();

    // Case-insensitive: normalize to lowercase
    std::string val;
    val.reserve(raw.size());
    for (char c : raw) val += (char)std::tolower(static_cast<unsigned char>(c));

    auto it = s_keywords.find(val);
    TokenType type = (it != s_keywords.end()) ? it->second : TokenType::IDENTIFIER;
    return { type, val, sl, sc };
}

// ─────────────────────────────────────────────────────────────────
//  Operator / punctuation scanner
//  Checks two-character sequences before single characters.
// ─────────────────────────────────────────────────────────────────
Token Lexer::scanOperator() {
    int sl = m_line, sc = m_col;
    char a = peek(0), b = peek(1);

    // ── Two-char operators ───────────────────────────────────────
    if (a == '=' && b == '=') { advance(); advance(); return { TokenType::OP_ASSIGN, "==", sl, sc }; }
    if (a == '=' && b == '>') { advance(); advance(); return { TokenType::OP_GTE,    "=>", sl, sc }; }
    if (a == '>' && b == '=') { advance(); advance(); return { TokenType::OP_GTE,    ">=", sl, sc }; }
    if (a == '!' && b == '=') { advance(); advance(); return { TokenType::OP_NEQ,    "!=", sl, sc }; }
    if (a == '<' && b == '=') { advance(); advance(); return { TokenType::OP_LTE,    "<=", sl, sc }; }
    if (a == '-' && b == '>') { advance(); advance(); return { TokenType::OP_ARROW,  "->", sl, sc }; }

    // ── Single-char operators & punctuation ─────────────────────
    advance();
    switch (a) {
        case '=': return { TokenType::OP_EQUAL,  "=",  sl, sc };
        case '>': return { TokenType::OP_GT,      ">",  sl, sc };
        case '<': return { TokenType::OP_LT,      "<",  sl, sc };
        case '+': return { TokenType::OP_PLUS,    "+",  sl, sc };
        case '-': return { TokenType::OP_MINUS,   "-",  sl, sc };
        case '*': return { TokenType::OP_STAR,    "*",  sl, sc };
        case '/': return { TokenType::OP_SLASH,   "/",  sl, sc };
        case '.': {
            char dotNext = peek();
            if (std::isalpha(static_cast<unsigned char>(dotNext)) || dotNext == '_')
                return { TokenType::OP_DOT, ".", sl, sc };
            return { TokenType::STMT_END, ".", sl, sc };
        }
        case ',': return { TokenType::COMMA,      ",",  sl, sc };
        case '(': return { TokenType::LPAREN,     "(",  sl, sc };
        case ')': return { TokenType::RPAREN,     ")",  sl, sc };
        case '[': return { TokenType::LBRACKET,   "[",  sl, sc };
        case ']': return { TokenType::RBRACKET,   "]",  sl, sc };
        case '{': return { TokenType::LBRACE,     "{",  sl, sc };
        case '}': return { TokenType::RBRACE,     "}",  sl, sc };
        default:  return { TokenType::UNKNOWN, std::string(1, a), sl, sc };
    }
}

// ─────────────────────────────────────────────────────────────────
//  HTML sub-lexer
//  Called after we've consumed "Html(" and emitted BUILTIN_HTML +
//  LPAREN.  Runs until the matching ')' that closes Html(...).
//
//  Grammar handled:
//    content  ::= (element | text)*
//    element  ::= '<' tagName attr* ('>' content '</' tagName '>'
//                                   | '/>')
//    attr     ::= name ('=' '"' value '"')?
//    text     ::= any chars that aren't '<' or ')'
//
//  The closing ')' of the Html() call is emitted as RPAREN and
//  we return to normal UCLang mode.
// ─────────────────────────────────────────────────────────────────
void Lexer::scanHtmlTokens(std::vector<Token>& out) {
    // Helper lambdas local to this function
    auto isNameChar = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c))
            || c == '-' || c == '_' || c == ':';
    };

    while (!atEnd()) {
        skipWhitespace();
        if (atEnd()) break;

        int sl = m_line, sc = m_col;
        char c = peek();

        // ── End of Html() block ───────────────────────────────────
        if (c == ')') {
            advance();
            out.push_back({ TokenType::RPAREN, ")", sl, sc });
            m_htmlMode = false;
            return;
        }

        // ── Tag ───────────────────────────────────────────────────
        if (c == '<') {
            advance();  // consume <
            sl = m_line; sc = m_col;

            if (peek() == '/') {
                // Closing tag  </tagname>
                advance();  // consume /
                std::string name;
                while (!atEnd() && isNameChar(peek())) name += advance();
                skipWhitespace();
                if (!atEnd() && peek() == '>') advance();  // consume >
                out.push_back({ TokenType::HTML_CLOSE, name, sl, sc });
            } else {
                // Opening tag  <tagname ...> or <tagname .../>
                std::string name;
                while (!atEnd() && isNameChar(peek())) name += advance();
                out.push_back({ TokenType::HTML_OPEN, name, sl, sc });

                // Scan attributes
                while (!atEnd()) {
                    skipWhitespace();
                    char nc = peek();

                    if (nc == '>') {
                        advance();
                        out.push_back({ TokenType::HTML_TAG_END, ">", m_line, m_col });
                        break;
                    }
                    if (nc == '/' && peek(1) == '>') {
                        advance(); advance();
                        out.push_back({ TokenType::HTML_SELF_CLOSE, "/>", m_line, m_col });
                        break;
                    }

                    // Attribute name
                    std::string attrName;
                    while (!atEnd() && isNameChar(peek())) attrName += advance();
                    if (attrName.empty()) { advance(); continue; }  // skip stray chars
                    out.push_back({ TokenType::HTML_ATTR_NAME, attrName, m_line, m_col });

                    skipWhitespace();
                    if (!atEnd() && peek() == '=') {
                        advance();  // consume =
                        skipWhitespace();
                        if (!atEnd() && peek() == '"') {
                            advance();  // consume opening "
                            std::string val;
                            while (!atEnd() && peek() != '"') val += advance();
                            if (!atEnd()) advance();  // consume closing "
                            out.push_back({ TokenType::HTML_ATTR_VAL, val, m_line, m_col });
                        }
                    }
                }
            }
            continue;
        }

        // ── Text node ─────────────────────────────────────────────
        std::string text;
        while (!atEnd() && peek() != '<' && peek() != ')') text += advance();
        // Only emit if non-whitespace
        bool blank = true;
        for (char ch : text) if (!std::isspace(static_cast<unsigned char>(ch))) { blank = false; break; }
        if (!blank)
            out.push_back({ TokenType::HTML_TEXT, text, sl, sc });
    }

    // If we reach here the Html() block was never closed
    throw std::runtime_error("Unterminated Html() block — missing closing )");
}

// ─────────────────────────────────────────────────────────────────
//  Main tokenise loop
// ─────────────────────────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(256);

    while (true) {
        skipWhitespace();
        if (atEnd()) break;

        // ── Block comments  ## ... ## ─────────────────────────────
        if (peek(0) == '#' && peek(1) == '#') {
            skipBlockComment();
            continue;
        }

        // ── Line comments  # ... ───────────────────────────────────
        if (peek(0) == '#') {
            while (!atEnd() && peek() != '\n') advance();
            continue;
        }

        int  sl = m_line, sc = m_col;
        char c  = peek();

        // ── String literal ────────────────────────────────────────
        if (c == '"') {
            tokens.push_back(scanString());
            continue;
        }

        // ── Number ────────────────────────────────────────────────
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(scanNumber());
            continue;
        }

        // ── Identifier / keyword ──────────────────────────────────
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            Token t = scanIdentifierOrKeyword();

            // Special case: Html keyword kicks us into HTML mode.
            // We consume the '(' ourselves and then hand off to the HTML sub-lexer.
            if (t.type == TokenType::BUILTIN_HTML) {
                tokens.push_back(t);
                skipWhitespace();
                if (!atEnd() && peek() == '(') {
                    advance();
                    tokens.push_back({ TokenType::LPAREN, "(", m_line, m_col });
                    m_htmlMode = true;
                    scanHtmlTokens(tokens);  // fills tokens until matching )
                }
                continue;
            }

            tokens.push_back(t);
            continue;
        }

        // ── Punctuation / operators ───────────────────────────────
        if (std::ispunct(static_cast<unsigned char>(c)) && c != '"' && c != '#') {
            tokens.push_back(scanOperator());
            continue;
        }

        // ── Anything else → UNKNOWN ───────────────────────────────
        tokens.push_back({ TokenType::UNKNOWN, std::string(1, advance()), sl, sc });
    }

    tokens.push_back({ TokenType::END_OF_FILE, "", m_line, m_col });
    return tokens;
}

// ─────────────────────────────────────────────────────────────────
//  Debug helper
// ─────────────────────────────────────────────────────────────────
const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::LIT_STRING:      return "LIT_STRING";
        case TokenType::LIT_INT:         return "LIT_INT";
        case TokenType::LIT_FLOAT:       return "LIT_FLOAT";
        case TokenType::LIT_BOOL:        return "LIT_BOOL";
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::TYPE_INT:        return "TYPE_INT";
        case TokenType::TYPE_FLOAT:      return "TYPE_FLOAT";
        case TokenType::TYPE_STRING:     return "TYPE_STRING";
        case TokenType::TYPE_BOOL:       return "TYPE_BOOL";
        case TokenType::KW_DEF:          return "KW_DEF";
        case TokenType::KW_RUN:          return "KW_RUN";
        case TokenType::KW_IF:           return "KW_IF";
        case TokenType::KW_THEN:         return "KW_THEN";
        case TokenType::KW_ELSE:         return "KW_ELSE";
        case TokenType::KW_RESPONSE:     return "KW_RESPONSE";
        case TokenType::KW_WHILE:        return "KW_WHILE";
        case TokenType::KW_BREAK:        return "KW_BREAK";
        case TokenType::KW_NEXT:         return "KW_NEXT";
        case TokenType::KW_CLASS:        return "KW_CLASS";
        case TokenType::KW_STRUCT:       return "KW_STRUCT";
        case TokenType::KW_INTERFACE:    return "KW_INTERFACE";
        case TokenType::KW_EXTENDS:      return "KW_EXTENDS";
        case TokenType::KW_IMPLEMENTS:   return "KW_IMPLEMENTS";
        case TokenType::KW_NEW:          return "KW_NEW";
        case TokenType::KW_THIS:         return "KW_THIS";
        case TokenType::KW_SUPER:        return "KW_SUPER";
        case TokenType::KW_PUBLIC:       return "KW_PUBLIC";
        case TokenType::KW_PRIVATE:      return "KW_PRIVATE";
        case TokenType::KW_PROTECTED:    return "KW_PROTECTED";
        case TokenType::KW_OVERRIDE:     return "KW_OVERRIDE";
        case TokenType::KW_VIRTUAL:      return "KW_VIRTUAL";
        case TokenType::KW_ABSTRACT:     return "KW_ABSTRACT";
        case TokenType::KW_STATIC:       return "KW_STATIC";
        case TokenType::KW_CONST:        return "KW_CONST";
        case TokenType::KW_YIELD:        return "KW_YIELD";
        case TokenType::KW_AS:           return "KW_AS";
        case TokenType::KW_REF:          return "KW_REF";
        case TokenType::KW_VAR:          return "KW_VAR";
        case TokenType::KW_NULL:         return "KW_NULL";
        case TokenType::KW_IMPORT:       return "KW_IMPORT";
        case TokenType::KW_NAMESPACE:    return "KW_NAMESPACE";
        case TokenType::KW_GET:          return "KW_GET";
        case TokenType::KW_SET:          return "KW_SET";
        case TokenType::BUILTIN_PRINT:   return "BUILTIN_PRINT";
        case TokenType::BUILTIN_INPUT:   return "BUILTIN_INPUT";
        case TokenType::BUILTIN_HTML:    return "BUILTIN_HTML";
        case TokenType::OP_ASSIGN:       return "OP_ASSIGN";
        case TokenType::OP_EQUAL:        return "OP_EQUAL";
        case TokenType::OP_GTE:          return "OP_GTE";
        case TokenType::OP_LTE:          return "OP_LTE";
        case TokenType::OP_GT:           return "OP_GT";
        case TokenType::OP_LT:           return "OP_LT";
        case TokenType::OP_NEQ:          return "OP_NEQ";
        case TokenType::OP_PLUS:         return "OP_PLUS";
        case TokenType::OP_MINUS:        return "OP_MINUS";
        case TokenType::OP_STAR:         return "OP_STAR";
        case TokenType::OP_SLASH:        return "OP_SLASH";
        case TokenType::OP_OR:           return "OP_OR";
        case TokenType::OP_AND:          return "OP_AND";
        case TokenType::STMT_END:        return "STMT_END";
        case TokenType::OP_DOT:          return "OP_DOT";
        case TokenType::OP_ARROW:        return "OP_ARROW";
        case TokenType::COMMA:           return "COMMA";
        case TokenType::LPAREN:          return "LPAREN";
        case TokenType::RPAREN:          return "RPAREN";
        case TokenType::LBRACKET:        return "LBRACKET";
        case TokenType::RBRACKET:        return "RBRACKET";
        case TokenType::LBRACE:          return "LBRACE";
        case TokenType::RBRACE:          return "RBRACE";
        case TokenType::HTML_OPEN:       return "HTML_OPEN";
        case TokenType::HTML_CLOSE:      return "HTML_CLOSE";
        case TokenType::HTML_SELF_CLOSE: return "HTML_SELF_CLOSE";
        case TokenType::HTML_TAG_END:    return "HTML_TAG_END";
        case TokenType::HTML_TEXT:       return "HTML_TEXT";
        case TokenType::HTML_ATTR_NAME:  return "HTML_ATTR_NAME";
        case TokenType::HTML_ATTR_VAL:   return "HTML_ATTR_VAL";
        case TokenType::END_OF_FILE:     return "EOF";
        default:                         return "UNKNOWN";
    }
}

} // namespace UCLang
