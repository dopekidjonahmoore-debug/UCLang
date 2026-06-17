#include "interpreter.h"
#include "core.h"
#include "lexer.h"
#include "parser.h"
#include "sdl_builtins.h"
#include "math_builtins.h"
#include "physics_builtins.h"
#include "gl_builtins.h"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>

namespace UCLang {

// ─── Control flow exceptions ───
struct BreakExc    {};
struct NextExc     {};
struct ReturnValue { Value value; };

// ==========================================================
//  Environment
// ==========================================================

Environment::Environment(std::shared_ptr<Environment> parent)
    : m_parent(std::move(parent)) {}

Value Environment::get(const std::string& name) const {
    auto it = m_vars.find(name);
    if (it != m_vars.end()) return it->second;
    if (m_parent) return m_parent->get(name);
    return std::monostate{};
}

void Environment::set(const std::string& name, const Value& v) {
    m_vars[name] = v;
}

void Environment::setOrUpdate(const std::string& name, const Value& v) {
    for (auto env = shared_from_this(); env; env = env->m_parent) {
        if (env->m_vars.count(name)) {
            env->m_vars[name] = v;
            return;
        }
    }
    m_vars[name] = v;
}

// ==========================================================
//  Interpreter
// ==========================================================

Interpreter::Interpreter()
    : m_env(std::make_shared<Environment>())
{
    inputFn  = [](const std::string& prompt) {
        printf("%s", prompt.c_str());
        fflush(stdout);
        std::string line;
        std::getline(std::cin, line);
        return line;
    };
    outputFn = [](const std::string& text) {
        printf("%s", text.c_str());
        fflush(stdout);
    };
    htmlFn   = [](const std::string&) {};
}

void Interpreter::registerFunction(const FuncDefNode& func) {
    m_functions[func.name] = &func;
}

// ─── Builtin native functions ───
static Value builtin_list_push(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("list_push(list val...): need at least a list");
    auto* list = std::get_if<ValueList>(&args[0]);
    if (!list) throw std::runtime_error("list_push: first arg must be a list");
    ValueList copy = *list;
    for (size_t i = 1; i < args.size(); ++i)
        copy.push_back(args[i]);
    return copy;
}
static Value builtin_list_pop(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("list_pop(list): need a list");
    auto* list = std::get_if<ValueList>(&args[0]);
    if (!list || list->empty()) throw std::runtime_error("list_pop: list is empty");
    ValueList copy = *list;
    Value val = copy.back();
    copy.pop_back();
    // Return both: last element is popped value, first is new list
    return val;
}
static Value builtin_list_len(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("list_len(list): need a list");
    auto* list = std::get_if<ValueList>(&args[0]);
    if (!list) throw std::runtime_error("list_len: arg must be a list");
    return Value(static_cast<int64_t>(list->size()));
}
static Value builtin_file_read(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("file_read(path): need a path");
    auto* path = std::get_if<std::string>(&args[0]);
    if (!path) throw std::runtime_error("file_read: path must be a string");
    std::ifstream f(*path);
    if (!f) return Value(std::string(""));
    std::stringstream buf;
    buf << f.rdbuf();
    return Value(buf.str());
}
static Value builtin_file_write(const std::vector<Value>& args) {
    if (args.size() < 2) throw std::runtime_error("file_write(path, content): need path and content");
    auto* path = std::get_if<std::string>(&args[0]);
    auto* content = std::get_if<std::string>(&args[1]);
    if (!path || !content) throw std::runtime_error("file_write: path and content must be strings");
    std::ofstream f(*path);
    if (!f) throw std::runtime_error("file_write: cannot open " + *path);
    f << *content;
    return std::monostate{};
}
static Value builtin_str_len(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("str_len(s): need a string");
    auto* s = std::get_if<std::string>(&args[0]);
    if (!s) throw std::runtime_error("str_len: arg must be a string");
    return Value(static_cast<int64_t>(s->size()));
}
static Value builtin_str_get(const std::vector<Value>& args) {
    if (args.size() < 2) throw std::runtime_error("str_get(s, i): need a string and index");
    auto* s = std::get_if<std::string>(&args[0]);
    auto* i = std::get_if<int64_t>(&args[1]);
    if (!s || !i) throw std::runtime_error("str_get: args must be string and int");
    if (*i < 0 || static_cast<size_t>(*i) >= s->size())
        return Value(std::string(""));
    return Value(std::string(1, (*s)[static_cast<size_t>(*i)]));
}
static Value builtin_str_slice(const std::vector<Value>& args) {
    if (args.size() < 3) throw std::runtime_error("str_slice(s, start, len): need string, start, len");
    auto* s = std::get_if<std::string>(&args[0]);
    auto* start = std::get_if<int64_t>(&args[1]);
    auto* len = std::get_if<int64_t>(&args[2]);
    if (!s || !start || !len) throw std::runtime_error("str_slice: args must be string, int, int");
    int64_t b = *start;
    int64_t e = b + *len;
    if (b < 0) b = 0;
    if (e > static_cast<int64_t>(s->size())) e = static_cast<int64_t>(s->size());
    if (b >= e) return Value(std::string(""));
    return Value(s->substr(static_cast<size_t>(b), static_cast<size_t>(e - b)));
}
static Value builtin_int_to_str(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("int_to_str(n): need an int");
    auto* i = std::get_if<int64_t>(&args[0]);
    if (i) return Value(std::to_string(*i));
    auto* f = std::get_if<double>(&args[0]);
    if (f) return Value(std::to_string(*f));
    auto* b = std::get_if<bool>(&args[0]);
    if (b) return Value(std::string(*b ? "1" : "0"));
    throw std::runtime_error("int_to_str: arg must be a number");
}
static Value builtin_list_get(const std::vector<Value>& args) {
    if (args.size() < 2) throw std::runtime_error("list_get(list, idx): need 2 args");
    auto* list = std::get_if<ValueList>(&args[0]);
    auto* idx = std::get_if<int64_t>(&args[1]);
    if (!list || !idx) throw std::runtime_error("list_get: args must be list and int");
    if (*idx < 0 || static_cast<size_t>(*idx) >= list->size())
        throw std::runtime_error("list_get: index out of bounds");
    return (*list)[static_cast<size_t>(*idx)];
}

static Value builtin_list_set(const std::vector<Value>& args) {
    if (args.size() < 3) throw std::runtime_error("list_set(list, idx, val): need 3 args");
    auto* list = std::get_if<ValueList>(&args[0]);
    auto* idx = std::get_if<int64_t>(&args[1]);
    if (!list || !idx) throw std::runtime_error("list_set: args must be list and int");
    if (*idx < 0 || static_cast<size_t>(*idx) >= list->size())
        throw std::runtime_error("list_set: index out of bounds");
    ValueList copy = *list;
    copy[static_cast<size_t>(*idx)] = args[2];
    return Value(copy);
}
static Value builtin_list_insert(const std::vector<Value>& args) {
    if (args.size() < 3) throw std::runtime_error("list_insert(list, idx, val): need 3 args");
    auto* list = std::get_if<ValueList>(&args[0]);
    auto* idx = std::get_if<int64_t>(&args[1]);
    if (!list || !idx) throw std::runtime_error("list_insert: args must be list and int");
    ValueList copy = *list;
    size_t uidx = static_cast<size_t>(*idx);
    if (uidx > copy.size()) uidx = copy.size();
    copy.insert(copy.begin() + static_cast<ptrdiff_t>(uidx), args[2]);
    return Value(copy);
}
static Value builtin_list_remove(const std::vector<Value>& args) {
    if (args.size() < 2) throw std::runtime_error("list_remove(list, idx): need 2 args");
    auto* list = std::get_if<ValueList>(&args[0]);
    auto* idx = std::get_if<int64_t>(&args[1]);
    if (!list || !idx) throw std::runtime_error("list_remove: args must be list and int");
    size_t uidx = static_cast<size_t>(*idx);
    if (*idx < 0 || uidx >= list->size())
        throw std::runtime_error("list_remove: index out of bounds");
    ValueList copy = *list;
    copy.erase(copy.begin() + static_cast<ptrdiff_t>(uidx));
    return Value(copy);
}
static Value builtin_list_contains(const std::vector<Value>& args) {
    if (args.size() < 2) throw std::runtime_error("list_contains(list, val): need 2 args");
    auto* list = std::get_if<ValueList>(&args[0]);
    if (!list) throw std::runtime_error("list_contains: first arg must be a list");
    for (const auto& item : *list) {
        // Simple equality check by type and value
        if (item.index() != args[1].index()) continue;
        if (auto* s = std::get_if<std::string>(&item)) {
            auto* s2 = std::get_if<std::string>(&args[1]);
            if (s2 && *s == *s2) return Value(true);
        } else if (auto* i = std::get_if<int64_t>(&item)) {
            auto* i2 = std::get_if<int64_t>(&args[1]);
            if (i2 && *i == *i2) return Value(true);
            auto* f2 = std::get_if<double>(&args[1]);
            if (f2 && *f2 == static_cast<double>(*i)) return Value(true);
        } else if (auto* f = std::get_if<double>(&item)) {
            auto* i2 = std::get_if<int64_t>(&args[1]);
            if (i2 && *i2 == static_cast<int64_t>(*f)) return Value(true);
            auto* f2 = std::get_if<double>(&args[1]);
            if (f2 && *f2 == *f) return Value(true);
        }
    }
    return Value(false);
}

static Value builtin_str_to_int(const std::vector<Value>& args) {
    if (args.empty()) throw std::runtime_error("str_to_int(s): need a string");
    auto* s = std::get_if<std::string>(&args[0]);
    if (!s) throw std::runtime_error("str_to_int: arg must be a string");
    char* end = nullptr;
    long long v = std::strtoll(s->c_str(), &end, 10);
    if (end && *end == '\0') return Value(static_cast<int64_t>(v));
    return Value(static_cast<int64_t>(0));
}

void Interpreter::registerBuiltins() {
    m_nativeFuncs["list_push"]   = builtin_list_push;
    m_nativeFuncs["list_pop"]    = builtin_list_pop;
    m_nativeFuncs["list_len"]    = builtin_list_len;
    m_nativeFuncs["list_get"]    = builtin_list_get;
    m_nativeFuncs["list_set"]    = builtin_list_set;
    m_nativeFuncs["list_insert"] = builtin_list_insert;
    m_nativeFuncs["list_remove"] = builtin_list_remove;
    m_nativeFuncs["list_contains"] = builtin_list_contains;
    m_nativeFuncs["list_index"]  = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("list_index(list, val): need 2 args");
        auto* list = std::get_if<ValueList>(&args[0]);
        if (!list) throw std::runtime_error("list_index: first arg must be a list");
        for (size_t i = 0; i < list->size(); i++) {
            const auto& item = (*list)[i];
            if (item.index() != args[1].index()) continue;
            if (auto* s = std::get_if<std::string>(&item)) {
                auto* s2 = std::get_if<std::string>(&args[1]);
                if (s2 && *s == *s2) return Value(static_cast<int64_t>(i));
            } else if (auto* iv = std::get_if<int64_t>(&item)) {
                auto* i2 = std::get_if<int64_t>(&args[1]);
                if (i2 && *iv == *i2) return Value(static_cast<int64_t>(i));
            } else if (auto* fv = std::get_if<double>(&item)) {
                auto* f2 = std::get_if<double>(&args[1]);
                if (f2 && *fv == *f2) return Value(static_cast<int64_t>(i));
            } else if (auto* bv = std::get_if<bool>(&item)) {
                auto* b2 = std::get_if<bool>(&args[1]);
                if (b2 && *bv == *b2) return Value(static_cast<int64_t>(i));
            }
        }
        return Value(static_cast<int64_t>(-1));
    };
    m_nativeFuncs["call_func"]   = [this](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("call_func(name,...): need at least function name");
        auto* name = std::get_if<std::string>(&args[0]);
        if (!name) throw std::runtime_error("call_func: first arg must be string name");
        // Normalise to lowercase (UCLang identifiers are case-insensitive)
        std::string lname; for (char c : *name) lname += (char)std::tolower((unsigned char)c);
        auto it = m_functions.find(lname);
        if (it == m_functions.end()) throw std::runtime_error("call_func: unknown function " + *name);
        const FuncDefNode* func = it->second;
        auto localEnv = std::make_shared<Environment>(m_env);
        for (size_t i = 1; i < args.size() && (i-1) < func->params.size(); ++i)
            localEnv->set(func->params[i-1], args[i]);
        auto oldEnv = m_env;
        m_env = localEnv;
        try {
            for (const auto& stmt : func->body)
                executeNode(*stmt);
        } catch (ReturnValue& rv) {
            m_env = oldEnv;
            return rv.value;
        }
        m_env = oldEnv;
        return std::monostate{};
    };
    m_nativeFuncs["run_file"] = [this](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("run_file(path): need file path");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("run_file: path must be string");
        std::string resolved = UCLang::resolveUclangPath(*path);
        std::ifstream f(resolved);
        if (!f) throw std::runtime_error("run_file: cannot open " + *path);
        std::stringstream buf; buf << f.rdbuf();
        std::string src = buf.str();
        Lexer lexer(src);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        auto& prog = std::get<ProgramNode>(ast->data);
        for (auto& stmt : prog.stmts) {
            if (auto* fn = std::get_if<FuncDefNode>(&stmt->data))
                registerFunction(*fn);
        }
        for (auto& stmt : prog.stmts)
            executeNode(*stmt);
        m_modules.push_back(std::move(ast)); // keep alive for call_func
        return std::monostate{};
    };
    m_nativeFuncs["file_read"]   = builtin_file_read;
    m_nativeFuncs["file_write"]  = builtin_file_write;
    m_nativeFuncs["str_len"]     = builtin_str_len;
    m_nativeFuncs["str_get"]     = builtin_str_get;
    m_nativeFuncs["str_slice"]   = builtin_str_slice;
    m_nativeFuncs["int_to_str"]  = builtin_int_to_str;
    m_nativeFuncs["str_to_int"]  = builtin_str_to_int;
    register_sdl_natives(m_nativeFuncs);
    register_math_natives(m_nativeFuncs);
    register_physics_natives(m_nativeFuncs);
    register_gl_natives(m_nativeFuncs);
    register_shader_natives(m_nativeFuncs);

    m_nativeFuncs["print"] = [this](const std::vector<Value>& args) -> Value {
        std::string s;
        for (const auto& a : args) {
            if (auto* v = std::get_if<std::string>(&a)) s += *v;
            else if (auto* v = std::get_if<int64_t>(&a)) s += std::to_string(*v);
            else if (auto* v = std::get_if<double>(&a)) s += std::to_string(*v);
            else if (auto* v = std::get_if<bool>(&a)) s += *v ? "true" : "false";
        }
        outputFn(s);
        return args.empty() ? Value() : args[0];
    };
    m_nativeFuncs["input"] = [this](const std::vector<Value>& args) -> Value {
        std::string prompt = args.empty() ? "" : (std::get_if<std::string>(&args[0]) ? *std::get_if<std::string>(&args[0]) : "");
        std::string line = inputFn ? inputFn(prompt) : "";
        return Value(line);
    };
}

Value Interpreter::execute(const Node& node) {
    return executeNode(node);
}

Value Interpreter::executeNode(const Node& node) {
    return std::visit(Visitor{
        [&](const ProgramNode& n)    { return execProgram(n); },
        [&](const FuncDefNode& n)    { return execFuncDef(n); },
        [&](const FuncCallNode& n)   { return execFuncCall(n, m_env); },
        [&](const IfNode& n)         { return execIf(n); },
        [&](const WhileNode& n)      { return execWhile(n); },
        [&](const BreakNode&) -> Value { throw BreakExc{}; },
        [&](const NextNode&) -> Value  { throw NextExc{}; },
        [&](const PrintNode& n)      { return execPrint(n); },
        [&](const InputNode& n)      { return execInput(n); },
        [&](const AssignNode& n)     { return execAssign(n); },
        [&](const MultiAssignNode& n) { return execMultiAssign(n); },
        [&](const TypeCheckNode& n)  { return execTypeCheck(n); },
        [&](const BinaryOpNode& n)   { return execBinaryOp(n); },
        [&](const LiteralNode& n)    { return execLiteral(n); },
        [&](const IdentifierNode& n) { return execIdentifier(n); },
        [&](const ListLiteralNode& n){ return execListLiteral(n); },
        [&](const ListGetNode& n)    { return execListGet(n); },
        [&](const HtmlNode& n)       { return execHtml(n); },
        [&](const ReturnNode& n)     { return execReturn(n); },
        [&](const GameLoopNode& n)   { return execGameLoop(n); },
        [&](std::monostate)          { return Value(); }
    }, node.data);
}

Value Interpreter::execProgram(const ProgramNode& n) {
    for (const auto& stmt : n.stmts)
        executeNode(*stmt);
    return std::monostate{};
}

Value Interpreter::execFuncDef(const FuncDefNode& n) {
    m_functions[n.name] = &n;
    return std::monostate{};
}

Value Interpreter::execFuncCall(const FuncCallNode& n, std::shared_ptr<Environment> callerEnv) {
    // Check native builtins first
    {
        auto it = m_nativeFuncs.find(n.name);
        if (it != m_nativeFuncs.end()) {
            std::vector<Value> args;
            for (const auto& a : n.args)
                args.push_back(evaluateExpr(*a));
            return it->second(args);
        }
    }

    auto it = m_functions.find(n.name);
    if (it == m_functions.end())
        throw std::runtime_error("Unknown function: " + n.name);

    const FuncDefNode* func = it->second;
    auto localEnv = std::make_shared<Environment>(callerEnv);

    for (size_t i = 0; i < func->params.size() && i < n.args.size(); ++i)
        localEnv->set(func->params[i], evaluateExpr(*n.args[i]));

    auto oldEnv = m_env;
    m_env = localEnv;

    try {
        for (const auto& stmt : func->body)
            executeNode(*stmt);
    } catch (ReturnValue& rv) {
        m_env = oldEnv;
        return rv.value;
    }

    m_env = oldEnv;
    return std::monostate{};
}

Value Interpreter::execIf(const IfNode& n) {
    Value cond = evaluateExpr(*n.cond);
    bool isTrue = false;
    if (auto* b = std::get_if<bool>(&cond)) isTrue = *b;
    else if (auto* i = std::get_if<int64_t>(&cond)) isTrue = *i != 0;
    else if (auto* f = std::get_if<double>(&cond)) isTrue = *f != 0.0;
    else if (auto* s = std::get_if<std::string>(&cond)) isTrue = !s->empty();

    if (isTrue) {
        for (const auto& stmt : n.thenBody) executeNode(*stmt);
    } else {
        for (const auto& stmt : n.elseBody) executeNode(*stmt);
    }
    return std::monostate{};
}

Value Interpreter::execWhile(const WhileNode& n) {
    while (true) {
        Value cond = evaluateExpr(*n.cond);
        bool isTrue = false;
        if (auto* b = std::get_if<bool>(&cond)) isTrue = *b;
        else if (auto* i = std::get_if<int64_t>(&cond)) isTrue = *i != 0;
        else if (auto* f = std::get_if<double>(&cond)) isTrue = *f != 0.0;
        else if (auto* s = std::get_if<std::string>(&cond)) isTrue = !s->empty();

        if (!isTrue) break;

        try {
            for (const auto& stmt : n.body) executeNode(*stmt);
        } catch (BreakExc&) {
            break;
        } catch (NextExc&) {
            continue;
        }
    }
    return std::monostate{};
}

static void valueToString(const Value& val, std::string& out) {
    std::visit(Visitor{
        [&](std::monostate)        { out += "null"; },
        [&](int64_t v)            { out += std::to_string(v); },
        [&](double v)             { out += std::to_string(v); },
        [&](const std::string& v) { out += v; },
        [&](bool v)               { out += v ? "true" : "false"; },
        [&](const ValueList& v) {
            out += "[";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i > 0) out += ", ";
                valueToString(v[i], out);
            }
            out += "]";
        }
    }, val);
}

Value Interpreter::execPrint(const PrintNode& n) {
    Value val = evaluateExpr(*n.value);
    std::string s;
    valueToString(val, s);
    outputFn(s);
    return val;
}

Value Interpreter::execInput(const InputNode& n) {
    Value promptVal = evaluateExpr(*n.prompt);
    std::string promptStr;
    if (auto* s = std::get_if<std::string>(&promptVal))
        promptStr = *s;
    else if (auto* i = std::get_if<int64_t>(&promptVal))
        promptStr = std::to_string(*i);

    std::string line = (inputFn ? inputFn(promptStr) : "");

    Value val = line;
    if (!line.empty()) {
        if (line.find('.') != std::string::npos) {
            char* end = nullptr;
            double fv = std::strtod(line.c_str(), &end);
            if (end && *end == '\0') val = fv;
        } else {
            char* end = nullptr;
            long iv = std::strtol(line.c_str(), &end, 10);
            if (end && *end == '\0') val = static_cast<int64_t>(iv);
        }
    }

    m_env->setOrUpdate(n.responseVar, val);
    return val;
}

Value Interpreter::execAssign(const AssignNode& n) {
    Value val = evaluateExpr(*n.value);
    m_env->setOrUpdate(n.var, val);
    return val;
}

Value Interpreter::execMultiAssign(const MultiAssignNode& n) {
    Value val = evaluateExpr(*n.value);
    auto* list = std::get_if<ValueList>(&val);
    if (!list)
        throw std::runtime_error("Multi-assign: value must be a list");
    for (size_t i = 0; i < n.vars.size() && i < list->size(); i++)
        m_env->setOrUpdate(n.vars[i], (*list)[i]);
    return val;
}

Value Interpreter::execTypeCheck(const TypeCheckNode& n) {
    Value val = evaluateExpr(*n.expr);
    bool result = false;
    if (auto* i = std::get_if<int64_t>(&val)) result = (n.typeName == "int");
    else if (auto* f = std::get_if<double>(&val)) result = (n.typeName == "float");
    else if (auto* s = std::get_if<std::string>(&val)) result = (n.typeName == "string");
    else if (auto* b = std::get_if<bool>(&val)) result = (n.typeName == "bool");
    return Value(result);
}

Value Interpreter::execBinaryOp(const BinaryOpNode& n) {
    Value l = evaluateExpr(*n.left);
    Value r = evaluateExpr(*n.right);

    if (n.op == "+" || n.op == "-" || n.op == "*" || n.op == "/") {
        if (auto* li = std::get_if<int64_t>(&l)) {
            if (auto* ri = std::get_if<int64_t>(&r)) {
                if (n.op == "+") return Value(*li + *ri);
                if (n.op == "-") return Value(*li - *ri);
                if (n.op == "*") return Value(*li * *ri);
                if (n.op == "/") return Value(*li / *ri);
            }
            if (auto* rf = std::get_if<double>(&r)) {
                double lf = static_cast<double>(*li);
                if (n.op == "+") return Value(lf + *rf);
                if (n.op == "-") return Value(lf - *rf);
                if (n.op == "*") return Value(lf * *rf);
                if (n.op == "/") return Value(lf / *rf);
            }
        }
        if (auto* lf = std::get_if<double>(&l)) {
            if (auto* ri = std::get_if<int64_t>(&r)) {
                double rf = static_cast<double>(*ri);
                if (n.op == "+") return Value(*lf + rf);
                if (n.op == "-") return Value(*lf - rf);
                if (n.op == "*") return Value(*lf * rf);
                if (n.op == "/") return Value(*lf / rf);
            }
            if (auto* rf = std::get_if<double>(&r)) {
                if (n.op == "+") return Value(*lf + *rf);
                if (n.op == "-") return Value(*lf - *rf);
                if (n.op == "*") return Value(*lf * *rf);
                if (n.op == "/") return Value(*lf / *rf);
            }
        }
        // String concatenation with +
        if (n.op == "+") {
            if (auto* ls = std::get_if<std::string>(&l)) {
                if (auto* rs = std::get_if<std::string>(&r))
                    return Value(*ls + *rs);
            }
        }
        throw std::runtime_error("Type mismatch in arithmetic: " + n.op);
    }

    // Boolean operators
    if (n.op == "or") {
        bool lb = false;
        if (auto* b = std::get_if<bool>(&l)) lb = *b;
        else if (auto* i = std::get_if<int64_t>(&l)) lb = *i != 0;
        else if (auto* f = std::get_if<double>(&l)) lb = *f != 0.0;
        else if (auto* s = std::get_if<std::string>(&l)) lb = !s->empty();
        if (lb) return Value(true);
        bool rb = false;
        if (auto* b = std::get_if<bool>(&r)) rb = *b;
        else if (auto* i = std::get_if<int64_t>(&r)) rb = *i != 0;
        else if (auto* f = std::get_if<double>(&r)) rb = *f != 0.0;
        else if (auto* s = std::get_if<std::string>(&r)) rb = !s->empty();
        return Value(rb);
    }
    if (n.op == "and") {
        bool lb = false;
        if (auto* b = std::get_if<bool>(&l)) lb = *b;
        else if (auto* i = std::get_if<int64_t>(&l)) lb = *i != 0;
        else if (auto* f = std::get_if<double>(&l)) lb = *f != 0.0;
        else if (auto* s = std::get_if<std::string>(&l)) lb = !s->empty();
        if (!lb) return Value(false);
        bool rb = false;
        if (auto* b = std::get_if<bool>(&r)) rb = *b;
        else if (auto* i = std::get_if<int64_t>(&r)) rb = *i != 0;
        else if (auto* f = std::get_if<double>(&r)) rb = *f != 0.0;
        else if (auto* s = std::get_if<std::string>(&r)) rb = !s->empty();
        return Value(rb);
    }

    // Comparisons
    if (n.op == "=") {
        if (l.index() != r.index()) return Value(false);
        if (auto* li = std::get_if<int64_t>(&l))
            return Value(*li == std::get<int64_t>(r));
        if (auto* lf = std::get_if<double>(&l))
            return Value(*lf == std::get<double>(r));
        if (auto* ls = std::get_if<std::string>(&l))
            return Value(*ls == std::get<std::string>(r));
        if (auto* lb = std::get_if<bool>(&l))
            return Value(*lb == std::get<bool>(r));
        return Value(true);
    }
    if (n.op == "!=") {
        Value eq;
        if (l.index() != r.index()) eq = Value(false);
        else {
            if (auto* li = std::get_if<int64_t>(&l)) eq = Value(*li == std::get<int64_t>(r));
            else if (auto* lf = std::get_if<double>(&l)) eq = Value(*lf == std::get<double>(r));
            else if (auto* ls = std::get_if<std::string>(&l)) eq = Value(*ls == std::get<std::string>(r));
            else if (auto* lb = std::get_if<bool>(&l)) eq = Value(*lb == std::get<bool>(r));
            else eq = Value(true); // same monostate
        }
        return Value(!std::get<bool>(eq));
    }

    auto cmp = [&](auto lv, auto rv) -> Value {
        if (n.op == ">")  return Value(lv > rv);
        if (n.op == "<")  return Value(lv < rv);
        if (n.op == "=>" || n.op == ">=") return Value(lv >= rv);
        if (n.op == "<=") return Value(lv <= rv);
        throw std::runtime_error("Unknown operator: " + n.op);
    };

    if (auto* li = std::get_if<int64_t>(&l)) {
        if (auto* ri = std::get_if<int64_t>(&r)) return cmp(*li, *ri);
        if (auto* rf = std::get_if<double>(&r)) return cmp(static_cast<double>(*li), *rf);
    }
    if (auto* lf = std::get_if<double>(&l)) {
        if (auto* ri = std::get_if<int64_t>(&r)) return cmp(*lf, static_cast<double>(*ri));
        if (auto* rf = std::get_if<double>(&r)) return cmp(*lf, *rf);
    }
    if (auto* ls = std::get_if<std::string>(&l)) {
        if (auto* rs = std::get_if<std::string>(&r)) return cmp(*ls, *rs);
    }

    throw std::runtime_error("Type mismatch in comparison: " + n.op);
}

Value Interpreter::execLiteral(const LiteralNode& n) {
    return std::visit(Visitor{
        [&](int64_t v)  { return Value(v); },
        [&](double v)   { return Value(v); },
        [&](const std::string& v) { return Value(v); },
        [&](bool v)     { return Value(v); }
    }, n.value);
}

Value Interpreter::execIdentifier(const IdentifierNode& n) {
    return m_env->get(n.name);
}

Value Interpreter::execListLiteral(const ListLiteralNode& n) {
    std::vector<Value> result;
    for (const auto& elem : n.elements)
        result.push_back(evaluateExpr(*elem));
    return Value(ValueList{result});
}

Value Interpreter::execListGet(const ListGetNode& n) {
    Value listVal = evaluateExpr(*n.list);
    Value idxVal  = evaluateExpr(*n.index);

    auto* list = std::get_if<ValueList>(&listVal);
    if (!list) throw std::runtime_error("Cannot index non-list value");

    int64_t idx;
    if (auto* i = std::get_if<int64_t>(&idxVal))
        idx = *i;
    else
        throw std::runtime_error("List index must be an integer");

    if (idx < 0 || (size_t)idx >= list->size())
        throw std::runtime_error("List index out of bounds");

    return (*list)[(size_t)idx];
}

Value Interpreter::execHtml(const HtmlNode& n) {
    htmlFn(n.rawHtml);
    return std::monostate{};
}

Value Interpreter::execReturn(const ReturnNode& n) {
    Value val = evaluateExpr(*n.value);
    throw ReturnValue{val};
}

Value Interpreter::execGameLoop(const GameLoopNode& n) {
    auto updateFn = [this, &n]() {
        for (const auto& stmt : n.updateBody)
            executeNode(*stmt);
    };
    auto drawFn = [this, &n]() {
        for (const auto& stmt : n.drawBody)
            executeNode(*stmt);
    };
    run_sdl_game_loop(updateFn, drawFn);
    return std::monostate{};
}

void Interpreter::execStmts(const std::vector<NodePtr>& stmts) {
    for (const auto& stmt : stmts)
        executeNode(*stmt);
}

Value Interpreter::evaluateExpr(const Node& node) {
    return std::visit(Visitor{
        [&](const BinaryOpNode& n)   { return execBinaryOp(n); },
        [&](const LiteralNode& n)    { return execLiteral(n); },
        [&](const IdentifierNode& n) { return execIdentifier(n); },
        [&](const FuncCallNode& n)   { return execFuncCall(n, m_env); },
        [&](const ListLiteralNode& n){ return execListLiteral(n); },
        [&](const ListGetNode& n)    { return execListGet(n); },
        [&](const PrintNode& n)      { return execPrint(n); },
        [&](const auto&) -> Value    { throw std::runtime_error("Expression expected"); }
    }, node.data);
}

} // namespace UCLang
