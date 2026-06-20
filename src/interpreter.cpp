#include "interpreter.h"
#include "core.h"
#include "lexer.h"
#include "parser.h"
#include "sdl_builtins.h"
#include "math_builtins.h"
#include "physics_builtins.h"
#include "gl_builtins.h"
#include "entity_builtins.h"
#include "serialization_builtins.h"
#include "animation_builtins.h"
#include "asset_builtins.h"
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

static void valueToString(const Value& val, std::string& out);

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
    register_entity_natives(m_nativeFuncs);
    register_serialization_natives(m_nativeFuncs);
    register_animation_natives(m_nativeFuncs);
    register_asset_natives(m_nativeFuncs);

    // ─── Coroutine builtins ──
    m_nativeFuncs["coro_start"] = [this](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::get_if<std::string>(&args[0]))
            throw std::runtime_error("coro_start(name, ...): need function name");
        std::string fname = *std::get_if<std::string>(&args[0]);
        auto fit = m_functions.find(fname);
        if (fit == m_functions.end())
            throw std::runtime_error("coro_start: unknown function " + fname);
        auto cd = std::make_shared<CoroutineData>();
        cd->funcName = fname;
        cd->funcDef = fit->second;
        cd->paramNames = fit->second->params;
        for (size_t i = 1; i < args.size(); ++i)
            cd->argValues.push_back(args[i]);
        cd->yieldStep = 0;
        cd->done = false;
        return Value(cd);
    };

    m_nativeFuncs["coro_resume"] = [this](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::get_if<std::shared_ptr<CoroutineData>>(&args[0]))
            throw std::runtime_error("coro_resume(coro, ...): need coroutine");
        auto cd = *std::get_if<std::shared_ptr<CoroutineData>>(&args[0]);
        if (cd->done) return Value();
        int step = cd->yieldStep;
        size_t nParams = cd->paramNames.size();
        size_t nExtra = args.size() - 1;
        auto localEnv = std::make_shared<Environment>(m_env);
        // Set params: first params match initial args, last param gets extra resume args
        for (size_t i = 0; i < nParams && i < cd->argValues.size(); ++i)
            localEnv->set(cd->paramNames[i], cd->argValues[i]);
        // Set yield step as a variable the function can check
        localEnv->set("__yield_step", Value((int64_t)step));
        // Extra resume args passed as __resume_val
        if (nExtra > 0)
            localEnv->set("__resume_val", args[1]);
        // Call the function
        auto oldEnv = m_env;
        m_env = localEnv;
        try {
            for (const auto& stmt : cd->funcDef->body)
                executeNode(*stmt);
        } catch (ReturnValue& rv) {
            m_env = oldEnv;
            cd->done = true;
            return rv.value;
        } catch (YieldSignal& ys) {
            m_env = oldEnv;
            cd->yieldStep = step + 1;
            return ys.value;
        }
        m_env = oldEnv;
        cd->done = true;
        return Value();
    };

    m_nativeFuncs["coro_done"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::get_if<std::shared_ptr<CoroutineData>>(&args[0]))
            return Value(true);
        auto cd = *std::get_if<std::shared_ptr<CoroutineData>>(&args[0]);
        return Value(cd->done);
    };

    m_nativeFuncs["coro_step"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::get_if<std::shared_ptr<CoroutineData>>(&args[0]))
            return Value((int64_t)0);
        auto cd = *std::get_if<std::shared_ptr<CoroutineData>>(&args[0]);
        return Value((int64_t)cd->yieldStep);
    };

    m_nativeFuncs["print"] = [this](const std::vector<Value>& args) -> Value {
        std::string s;
        for (const auto& a : args)
            valueToString(a, s);
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
        // OOP nodes
        [&](const ClassDefNode& n)   { return execClassDef(n); },
        [&](const NewExprNode& n)    { return execNewExpr(n); },
        [&](const ThisExprNode&)     { return m_currentThis; },
        [&](const SuperExprNode&)    { return m_currentThis; },
        [&](const MemberCallNode& n) { return execMemberCall(n); },
        [&](const MemberGetNode& n)  { return execMemberGet(n); },
        [&](const MemberSetNode& n)  { return execMemberSet(n); },
        [&](const NullNode&)         { return Value(); },
        [&](const YieldNode& n)      { return execYield(n); },
        [&](const ImportNode& n)     { return execImport(n); },
        [&](const AsCastNode& n)     { return evaluateExpr(*n.expr); },
        [&](std::monostate)          { return Value(); }
    }, node.data);
}

Value Interpreter::execProgram(const ProgramNode& n) {
    for (size_t i = 0; i < n.stmts.size(); ++i) {
        executeNode(*n.stmts[i]);
    }
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
    } catch (YieldSignal& ys) {
        m_env = oldEnv;
        throw; // propagate yield up
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
        },
        [&](const std::shared_ptr<UCLangObjectData>& obj) {
            out += "Object(" + obj->className + ")";
        },
        [&](const Vec3& v) {
            out += "Vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        },
        [&](const Quat& q) {
            out += "Quat(" + std::to_string(q.x) + ", " + std::to_string(q.y) + ", " + std::to_string(q.z) + ", " + std::to_string(q.w) + ")";
        },
        [&](const Mat4&) {
            out += "Mat4";
        },
        [&](const std::shared_ptr<CoroutineData>& cd) {
            out += "Coroutine(" + cd->funcName + ")";
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
    else if (auto* obj = std::get_if<std::shared_ptr<UCLangObjectData>>(&val))
        result = ((*obj)->className == n.typeName);
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
        // Vec3 arithmetic
        if (auto* lv = std::get_if<Vec3>(&l)) {
            if (auto* rv = std::get_if<Vec3>(&r)) {
                if (n.op == "+") return Value(*lv + *rv);
                if (n.op == "-") return Value(*lv - *rv);
            }
            if (auto* rf = std::get_if<double>(&r)) {
                float f = (float)*rf;
                if (n.op == "*") return Value(*lv * f);
                if (n.op == "/") return Value(*lv / f);
            }
            if (auto* ri = std::get_if<int64_t>(&r)) {
                float f = (float)*ri;
                if (n.op == "*") return Value(*lv * f);
                if (n.op == "/") return Value(*lv / f);
            }
        }
        if (auto* lf = std::get_if<double>(&l)) {
            if (auto* rv = std::get_if<Vec3>(&r)) {
                float f = (float)*lf;
                if (n.op == "*") return Value(f * *rv);
            }
        }
        if (auto* li = std::get_if<int64_t>(&l)) {
            if (auto* rv = std::get_if<Vec3>(&r)) {
                float f = (float)*li;
                if (n.op == "*") return Value(f * *rv);
            }
        }
        // Quat arithmetic
        if (auto* lq = std::get_if<Quat>(&l)) {
            if (auto* rq = std::get_if<Quat>(&r)) {
                if (n.op == "*") return Value(*lq * *rq);
            }
        }
        // Mat4 arithmetic
        if (auto* lm = std::get_if<Mat4>(&l)) {
            if (auto* rm = std::get_if<Mat4>(&r)) {
                if (n.op == "*") return Value(*lm * *rm);
            }
            if (auto* rv = std::get_if<Vec3>(&r)) {
                if (n.op == "*") {
                    glm::vec4 v4 = *lm * glm::vec4(*rv, 1.0f);
                    return Value(glm::vec3(v4) / v4.w);
                }
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
        if (auto* lo = std::get_if<std::shared_ptr<UCLangObjectData>>(&l))
            return Value(*lo == std::get<std::shared_ptr<UCLangObjectData>>(r));
        if (auto* lv = std::get_if<Vec3>(&l))
            return Value(*lv == std::get<Vec3>(r));
        if (auto* lq = std::get_if<Quat>(&l))
            return Value(*lq == std::get<Quat>(r));
        if (auto* lm = std::get_if<Mat4>(&l))
            return Value(*lm == std::get<Mat4>(r));
        if (auto* lc = std::get_if<std::shared_ptr<CoroutineData>>(&l))
            return Value(*lc == std::get<std::shared_ptr<CoroutineData>>(r));
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
            else if (auto* lo = std::get_if<std::shared_ptr<UCLangObjectData>>(&l))
                eq = Value(*lo == std::get<std::shared_ptr<UCLangObjectData>>(r));
            else if (auto* lv = std::get_if<Vec3>(&l)) eq = Value(*lv == std::get<Vec3>(r));
            else if (auto* lq = std::get_if<Quat>(&l)) eq = Value(*lq == std::get<Quat>(r));
            else if (auto* lm = std::get_if<Mat4>(&l)) eq = Value(*lm == std::get<Mat4>(r));
            else if (auto* lc = std::get_if<std::shared_ptr<CoroutineData>>(&l))
                eq = Value(*lc == std::get<std::shared_ptr<CoroutineData>>(r));
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

// ==========================================================
//  OOP Helpers
// ==========================================================

const ClassDefNode* Interpreter::findClass(const std::string& name) const {
    auto it = m_classes.find(name);
    if (it != m_classes.end()) {
        return it->second;
    }
    return nullptr;
}

const ClassMemberNode* Interpreter::findMethodInClass(const ClassDefNode* cls, const std::string& methodName) const {
    if (!cls) return nullptr;
    for (const auto& member : cls->members) {
        if (member.name == methodName && member.value &&
            std::holds_alternative<FuncDefNode>(member.value->data)) {
            return &member;
        }
    }
    // Search parent chain
    if (!cls->parent.empty()) {
        const ClassDefNode* parent = findClass(cls->parent);
        if (parent) return findMethodInClass(parent, methodName);
    }
    return nullptr;
}

// ==========================================================
//  Class definition  (store in registry)
// ==========================================================

Value Interpreter::execClassDef(const ClassDefNode& n) {
    // If this is a class that implements interfaces, verify all interface methods are implemented
    for (const auto& ifaceName : n.interfaces) {
        auto it = m_classes.find(ifaceName);
        if (it == m_classes.end())
            throw std::runtime_error("Unknown interface: " + ifaceName + " used by class " + n.name);
        const ClassDefNode* iface = it->second;
        // Check that every method in the interface has a matching implementation
        for (const auto& ifaceMember : iface->members) {
            if (!ifaceMember.value) continue;
            if (!std::holds_alternative<FuncDefNode>(ifaceMember.value->data)) continue;
            const ClassMemberNode* impl = findMethodInClass(&n, ifaceMember.name);
            if (!impl)
                throw std::runtime_error("Class " + n.name + " does not implement interface method: " + ifaceMember.name);
        }
    }
    m_classes[n.name] = &n;
    return std::monostate{};
}

const ClassDefNode* Interpreter::getCurrentClassDef() const {
    if (m_currentThisClassName.empty()) return nullptr;
    return findClass(m_currentThisClassName);
}

const ClassDefNode* Interpreter::findFieldDefiningClass(const ClassDefNode* startClass, const std::string& fieldName) const {
    if (!startClass) return nullptr;
    // Walk the inheritance chain: start from startClass, then parent, grandparent...
    const ClassDefNode* cur = startClass;
    while (cur) {
        for (const auto& member : cur->members) {
            if (member.name == fieldName) {
                return cur; // found the defining class
            }
        }
        if (cur->parent.empty()) break;
        cur = findClass(cur->parent);
    }
    return nullptr;
}

const ClassDefNode* Interpreter::findMethodDefiningClass(const ClassDefNode* startClass, const std::string& methodName) const {
    if (!startClass) return nullptr;
    const ClassDefNode* cur = startClass;
    while (cur) {
        for (const auto& member : cur->members) {
            if (member.name == methodName && member.value &&
                std::holds_alternative<FuncDefNode>(member.value->data)) {
                return cur;
            }
        }
        if (cur->parent.empty()) break;
        cur = findClass(cur->parent);
    }
    return nullptr;
}

bool Interpreter::checkAccess(const std::string& fieldDefiningClass, const ClassMemberNode* member) const {
    if (!member) return true;
    if (member->visibility == "public") return true;
    if (member->visibility == "private") {
        // Only the class that defines this field/method can access it privately
        return m_currentDefiningClassName == fieldDefiningClass;
    }
    if (member->visibility == "protected") {
        // Allow from same class or subclasses of the defining class
        if (m_currentDefiningClassName.empty()) return true;
        if (m_currentDefiningClassName == fieldDefiningClass) return true;
        const ClassDefNode* cur = findClass(m_currentDefiningClassName);
        while (cur) {
            if (cur->name == fieldDefiningClass) return true;
            if (cur->parent.empty()) break;
            cur = findClass(cur->parent);
        }
        return false;
    }
    return true;
}

// ==========================================================
//  new expression   new ClassName(args.)
// ==========================================================

Value Interpreter::execNewExpr(const NewExprNode& n) {
    auto it = m_classes.find(n.className);
    if (it == m_classes.end())
        throw std::runtime_error("Unknown class: " + n.className);

    auto objData = std::make_shared<UCLangObjectData>();
    objData->className = n.className;

    // Walk inheritance chain to collect fields from parent classes
    std::vector<const ClassDefNode*> chain;
    const ClassDefNode* cur = it->second;
    while (cur) {
        chain.push_back(cur);
        if (!cur->parent.empty())
            cur = findClass(cur->parent);
        else
            cur = nullptr;
    }
    // Initialize fields from most-parent to most-child
    for (auto cls = chain.rbegin(); cls != chain.rend(); ++cls) {
        for (const auto& member : (*cls)->members) {
            if (member.value && std::holds_alternative<FuncDefNode>(member.value->data))
                continue; // skip methods
            if (objData->fields.count(member.name))
                continue; // child already set, skip (allow override defaults)
            if (member.value)
                objData->fields[member.name] = evaluateExpr(*member.value);
            else
                objData->fields[member.name] = std::monostate{};
        }
    }

    Value objVal = Value(std::move(objData));

    // Call init() if it exists with provided args
    const ClassMemberNode* initMethod = findMethodInClass(it->second, "init");
    if (initMethod) {
        const auto& funcNode = std::get<FuncDefNode>(initMethod->value->data);
        auto localEnv = std::make_shared<Environment>(m_env);
        localEnv->set("this", objVal);
        for (size_t i = 0; i < funcNode.params.size() && i < n.args.size(); ++i)
            localEnv->set(funcNode.params[i], evaluateExpr(*n.args[i]));
        auto oldThis = m_currentThis;
        auto oldEnv = m_env;
        auto oldThisCls = m_currentThisClassName;
        auto oldDefCls = m_currentDefiningClassName;
        m_currentThis = objVal;
        m_currentThisClassName = n.className;
        // init() defining class
        const ClassDefNode* initDefCls = findMethodDefiningClass(it->second, "init");
        m_currentDefiningClassName = initDefCls ? initDefCls->name : n.className;
        m_env = localEnv;
        try {
            for (const auto& stmt : funcNode.body)
                executeNode(*stmt);
        } catch (ReturnValue& rv) {
            m_env = oldEnv;
            m_currentThis = oldThis;
            m_currentThisClassName = oldThisCls;
            m_currentDefiningClassName = oldDefCls;
            return objVal;
        }
        m_env = oldEnv;
        m_currentThis = oldThis;
        m_currentThisClassName = oldThisCls;
        m_currentDefiningClassName = oldDefCls;
    } else if (!n.args.empty()) {
        throw std::runtime_error("Class " + n.className + " has no init constructor but args were provided");
    }

    return objVal;
}

// ==========================================================
//  Member call   obj.method(args.)
// ==========================================================

// Helper: extract a double from a Value (for native math method args)
static double toDouble(const Value& v) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    if (auto* i = std::get_if<int64_t>(&v)) return (double)*i;
    return 0.0;
}

Value Interpreter::execMemberCall(const MemberCallNode& n) {
    // Check if this is a super call
    bool superDispatch = std::holds_alternative<SuperExprNode>(n.object->data);
    Value objVal;
    if (superDispatch) {
        objVal = m_currentThis;
    } else {
        objVal = evaluateExpr(*n.object);
    }

    // ── Native Vec3 method calls ──
    if (auto* v3 = std::get_if<Vec3>(&objVal)) {
        const auto& mn = n.member;
        if (mn == "normalize") {
            return Value(glm::normalize(*v3));
        }
        if (mn == "length") {
            return Value((double)glm::length(*v3));
        }
        if (mn == "dot" && n.args.size() >= 1) {
            if (auto* other = std::get_if<Vec3>(&evaluateExpr(*n.args[0])))
                return Value((double)glm::dot(*v3, *other));
            throw std::runtime_error("Vec3.dot() expects a Vec3 argument");
        }
        if (mn == "cross" && n.args.size() >= 1) {
            if (auto* other = std::get_if<Vec3>(&evaluateExpr(*n.args[0])))
                return Value(glm::cross(*v3, *other));
            throw std::runtime_error("Vec3.cross() expects a Vec3 argument");
        }
        throw std::runtime_error("Vec3 has no method: " + mn);
    }

    // ── Native Quat method calls ──
    if (auto* q = std::get_if<Quat>(&objVal)) {
        const auto& mn = n.member;
        if (mn == "normalize")  return Value(glm::normalize(*q));
        if (mn == "conjugate")  return Value(glm::conjugate(*q));
        if (mn == "inverse")    return Value(glm::inverse(*q));
        throw std::runtime_error("Quat has no method: " + mn);
    }

    // ── Native Mat4 method calls ──
    if (auto* m4 = std::get_if<Mat4>(&objVal)) {
        const auto& mn = n.member;
        if (mn == "inverse") {
            return Value(glm::inverse(*m4));
        }
        if (mn == "transpose") {
            return Value(glm::transpose(*m4));
        }
        if (mn == "translate" && n.args.size() >= 3) {
            double x = toDouble(evaluateExpr(*n.args[0]));
            double y = toDouble(evaluateExpr(*n.args[1]));
            double z = toDouble(evaluateExpr(*n.args[2]));
            return Value(*m4 * glm::translate(glm::mat4(1.0f), glm::vec3((float)x, (float)y, (float)z)));
        }
        if (mn == "rotate" && n.args.size() >= 4) {
            double angle = toDouble(evaluateExpr(*n.args[0]));
            double ax = toDouble(evaluateExpr(*n.args[1]));
            double ay = toDouble(evaluateExpr(*n.args[2]));
            double az = toDouble(evaluateExpr(*n.args[3]));
            return Value(*m4 * glm::rotate(glm::mat4(1.0f), glm::radians((float)angle), glm::vec3((float)ax, (float)ay, (float)az)));
        }
        if (mn == "scale" && n.args.size() >= 3) {
            double x = toDouble(evaluateExpr(*n.args[0]));
            double y = toDouble(evaluateExpr(*n.args[1]));
            double z = toDouble(evaluateExpr(*n.args[2]));
            return Value(*m4 * glm::scale(glm::mat4(1.0f), glm::vec3((float)x, (float)y, (float)z)));
        }
        throw std::runtime_error("Mat4 has no method: " + mn);
    }

    // ── Namespace native function call: serialize.load_text(...) ──
    if (std::get_if<std::monostate>(&objVal)) {
        auto* idNode = std::get_if<IdentifierNode>(&n.object->data);
        if (idNode) {
            std::string fullName = idNode->name + "." + n.member;
            auto it = m_nativeFuncs.find(fullName);
            if (it != m_nativeFuncs.end()) {
                std::vector<Value> args;
                for (auto& arg : n.args)
                    args.push_back(evaluateExpr(*arg));
                return it->second(args);
            }
        }
    }

    // ── UCLang object method calls ──
    auto* objData = std::get_if<std::shared_ptr<UCLangObjectData>>(&objVal);
    if (!objData)
        throw std::runtime_error("Cannot call method on non-object value");

    const std::string& className = (*objData)->className;
    const ClassDefNode* classDef = findClass(className);
    if (!classDef)
        throw std::runtime_error("Unknown class: " + className);

    // Find method (start from parent class if super dispatch)
    const ClassMemberNode* method = nullptr;
    if (superDispatch && !classDef->parent.empty()) {
        const ClassDefNode* parentDef = findClass(classDef->parent);
        method = findMethodInClass(parentDef, n.member);
    } else {
        method = findMethodInClass(classDef, n.member);
    }

    if (!method)
        throw std::runtime_error("Unknown method: " + n.member + " in class " + className);

    const auto& funcNode = std::get<FuncDefNode>(method->value->data);

    auto localEnv = std::make_shared<Environment>(m_env);
    localEnv->set("this", objVal);

    for (size_t i = 0; i < funcNode.params.size() && i < n.args.size(); ++i)
        localEnv->set(funcNode.params[i], evaluateExpr(*n.args[i]));

    auto oldThis = m_currentThis;
    auto oldEnv = m_env;
    auto oldThisCls = m_currentThisClassName;
    auto oldDefCls = m_currentDefiningClassName;
    m_currentThis = objVal;
    m_currentThisClassName = className;
    // Find which class actually defines this method
    const ClassDefNode* methodDefCls = nullptr;
    if (superDispatch && !classDef->parent.empty()) {
        const ClassDefNode* parentDef = findClass(classDef->parent);
        methodDefCls = findMethodDefiningClass(parentDef, n.member);
    } else {
        methodDefCls = findMethodDefiningClass(classDef, n.member);
    }
    m_currentDefiningClassName = methodDefCls ? methodDefCls->name : className;
    m_env = localEnv;

    try {
        for (const auto& stmt : funcNode.body)
            executeNode(*stmt);
    } catch (ReturnValue& rv) {
        m_env = oldEnv;
        m_currentThis = oldThis;
        m_currentThisClassName = oldThisCls;
        m_currentDefiningClassName = oldDefCls;
        return rv.value;
    }

    m_env = oldEnv;
    m_currentThis = oldThis;
    m_currentThisClassName = oldThisCls;
    m_currentDefiningClassName = oldDefCls;
    return std::monostate{};
}

// ==========================================================
//  Member get   obj.field
// ==========================================================

Value Interpreter::execMemberGet(const MemberGetNode& n) {
    Value objVal = evaluateExpr(*n.object);

    // UCLang object field access
    if (auto* objData = std::get_if<std::shared_ptr<UCLangObjectData>>(&objVal)) {
        const std::string& className = (*objData)->className;
        const ClassDefNode* classDef = findClass(className);
        if (!classDef)
            throw std::runtime_error("Unknown class: " + className);

        // Walk inheritance chain to find which class defines this field
        const ClassDefNode* fieldClass = findFieldDefiningClass(classDef, n.member);
        if (fieldClass) {
            // Find the actual member node in the defining class
            for (const auto& member : fieldClass->members) {
                if (member.name == n.member) {
                    if (!checkAccess(fieldClass->name, &member))
                        throw std::runtime_error("Cannot access " + member.visibility + " field '" + n.member + "' from here");
                    break;
                }
            }
        }

        auto it = (*objData)->fields.find(n.member);
        if (it == (*objData)->fields.end())
            throw std::runtime_error("Unknown field: " + n.member);
        return it->second;
    }

    // ── Namespace native property access ──
    if (std::get_if<std::monostate>(&objVal)) {
        auto* idNode = std::get_if<IdentifierNode>(&n.object->data);
        if (idNode) {
            std::string fullName = idNode->name + "." + n.member;
            auto it = m_nativeFuncs.find(fullName);
            if (it != m_nativeFuncs.end())
                return it->second({});
        }
    }

    // Native Vec3 field access
    if (auto* v = std::get_if<Vec3>(&objVal)) {
        if (n.member == "x") return Value((double)v->x);
        if (n.member == "y") return Value((double)v->y);
        if (n.member == "z") return Value((double)v->z);
        throw std::runtime_error("Vec3 has no field: " + n.member);
    }

    // Native Quat field access
    if (auto* q = std::get_if<Quat>(&objVal)) {
        if (n.member == "x") return Value((double)q->x);
        if (n.member == "y") return Value((double)q->y);
        if (n.member == "z") return Value((double)q->z);
        if (n.member == "w") return Value((double)q->w);
        throw std::runtime_error("Quat has no field: " + n.member);
    }

    throw std::runtime_error("Cannot access field on non-object value");
}

// ==========================================================
//  Member set   obj.field == value
// ==========================================================

Value Interpreter::execMemberSet(const MemberSetNode& n) {
    Value objVal = evaluateExpr(*n.object);
    auto* objData = std::get_if<std::shared_ptr<UCLangObjectData>>(&objVal);
    if (!objData)
        throw std::runtime_error("Cannot set field on non-object value");

    // Walk inheritance chain to find which class defines this field
    const std::string& className = (*objData)->className;
    const ClassDefNode* classDef = findClass(className);
    if (classDef) {
        const ClassDefNode* fieldClass = findFieldDefiningClass(classDef, n.member);
        if (fieldClass) {
            for (const auto& member : fieldClass->members) {
                if (member.name == n.member) {
                    if (!checkAccess(fieldClass->name, &member))
                        throw std::runtime_error("Cannot access " + member.visibility + " field '" + n.member + "' from here");
                    break;
                }
            }
        }
    }

    Value val = evaluateExpr(*n.value);
    (*objData)->fields[n.member] = val;
    return val;
}

// ==========================================================
//  Import   import "path"
// ==========================================================

Value Interpreter::execImport(const ImportNode& n) {
    std::string resolved = UCLang::resolveUclangPath(n.path);
    std::ifstream f(resolved);
    if (!f) throw std::runtime_error("import: cannot open " + n.path);
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
    m_modules.push_back(std::move(ast));
    return std::monostate{};
}

Value Interpreter::execYield(const YieldNode& n) {
    Value val = evaluateExpr(*n.value);
    throw YieldSignal{val};
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
        // OOP expressions
        [&](const NewExprNode& n)    { return execNewExpr(n); },
        [&](const MemberCallNode& n) { return execMemberCall(n); },
        [&](const MemberGetNode& n)  { return execMemberGet(n); },
        [&](const NullNode&)         { return Value(); },
        [&](const ThisExprNode&)     { return m_currentThis; },
        [&](const AsCastNode& n)     { return evaluateExpr(*n.expr); },
        [&](const auto&) -> Value    { throw std::runtime_error("Expression expected"); }
    }, node.data);
}

} // namespace UCLang
