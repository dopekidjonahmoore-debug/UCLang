#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace UCLang {

// ─────────────────────────────────────────────────────────────────
//  Token Types
//  Every distinct thing the lexer can recognise gets a type here.
// ─────────────────────────────────────────────────────────────────
enum class TokenType {

    // ── Literals ──────────────────────────────────────────────────
    LIT_STRING,         // "hello world"
    LIT_INT,            // 42
    LIT_FLOAT,          // 3.14
    LIT_BOOL,           // true  /  false

    // ── Identifiers ───────────────────────────────────────────────
    IDENTIFIER,         // variable names, user-defined function names

    // ── Type names (used in type-check expressions) ───────────────
    TYPE_INT,           // Int
    TYPE_FLOAT,         // Float
    TYPE_STRING,        // String
    TYPE_BOOL,          // Bool

    // ── Language keywords ─────────────────────────────────────────
    KW_DEF,             // def   (inside [def name])
    KW_RUN,             // run   (run.[def name])
    KW_IF,              // If
    KW_THEN,            // then
    KW_ELSE,            // else
    KW_WHILE,           // while
    KW_BREAK,           // break
    KW_NEXT,            // next
    KW_RESPONSE,        // response  (inside Input())

    // ── Built-in functions ────────────────────────────────────────
    BUILTIN_PRINT,      // Print
    BUILTIN_INPUT,      // Input
    BUILTIN_HTML,       // Html   (triggers HTML mode)

    // ── Operators ─────────────────────────────────────────────────
    //   UCLang operator table:
    //     ==   assignment / capture       (var==value)
    //     =    equality / type-check      (var=Int)
    //     =>   greater-than-or-equal      (age=>18)
    //     <=   less-than-or-equal
    //     >    greater-than
    //     <    less-than  (outside HTML context)
    //     !=   not-equal
    //     +  -  *  /     arithmetic
    OP_ASSIGN,          // ==
    OP_EQUAL,           // =
    OP_GTE,             // =>
    OP_LTE,             // <=
    OP_GT,              // >
    OP_LT,              // <
    OP_NEQ,             // !=
    OP_PLUS,            // +
    OP_MINUS,           // -
    OP_STAR,            // *
    OP_SLASH,           // /
    OP_OR,              // or  (boolean)
    OP_AND,             // and (boolean)

    // ── Delimiters ────────────────────────────────────────────────
    STMT_END,           // .   statement terminator
    OP_DOT,             // .   member access (when followed by identifier: Window.open)
    COMMA,              // ,
    LPAREN,             // (
    RPAREN,             // )
    LBRACKET,           // [
    RBRACKET,           // ]

    // ── HTML-mode tokens (emitted when inside Html(...)) ──────────
    HTML_OPEN,          // <tagname
    HTML_CLOSE,         // </tagname>
    HTML_SELF_CLOSE,    // />
    HTML_TAG_END,       // >  (end of an opening tag's attribute list)
    HTML_TEXT,          // raw text between tags
    HTML_ATTR_NAME,     // attribute name
    HTML_ATTR_VAL,      // attribute value (after =)

    // ── Special ───────────────────────────────────────────────────
    END_OF_FILE,
    UNKNOWN
};

// ─────────────────────────────────────────────────────────────────
//  Token
// ─────────────────────────────────────────────────────────────────
struct Token {
    TokenType   type;
    std::string value;   // raw source text
    int         line;
    int         column;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), column(c) {}
};

// ─────────────────────────────────────────────────────────────────
//  Lexer
//  Converts a UCLang source string into a flat list of Tokens.
// ─────────────────────────────────────────────────────────────────
class Lexer {
public:
    explicit Lexer(std::string source);

    // Run the full tokenisation pass. Throws std::runtime_error on bad input.
    std::vector<Token> tokenize();

private:
    std::string m_source;
    size_t      m_pos      = 0;
    int         m_line     = 1;
    int         m_col      = 1;
    bool        m_htmlMode = false;   // true while scanning inside Html(...)

    // ── Low-level helpers ─────────────────────────────────────────
    char    peek(int offset = 0) const;
    char    advance();
    bool    atEnd() const;
    void    skipWhitespace();
    bool    skipBlockComment();        // ## ... ##

    // ── Scanners ──────────────────────────────────────────────────
    Token scanString();
    Token scanNumber();
    Token scanIdentifierOrKeyword();
    Token scanOperator();

    // HTML sub-lexer — called when m_htmlMode == true
    void  scanHtmlTokens(std::vector<Token>& out);

    // ── Keyword → TokenType table ─────────────────────────────────
    static const std::unordered_map<std::string, TokenType> s_keywords;
};

// Human-readable token type name (useful for error messages & debug)
const char* tokenTypeName(TokenType t);

} // namespace UCLang
