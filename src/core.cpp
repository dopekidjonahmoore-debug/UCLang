#include "core.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

struct UCLangVM {
    UCLangOutputFn outputFn = nullptr;
    UCLangInputFn  inputFn  = nullptr;
    void* outputUserdata = nullptr;
    void* inputUserdata  = nullptr;
    std::string lastError;
};

static thread_local std::string tls_error;

UCLangVM* uclang_vm_new(void) {
    return new UCLangVM();
}

void uclang_vm_free(UCLangVM* vm) {
    delete vm;
}

void uclang_vm_set_output(UCLangVM* vm, UCLangOutputFn fn, void* userdata) {
    vm->outputFn = fn;
    vm->outputUserdata = userdata;
}

void uclang_vm_set_input(UCLangVM* vm, UCLangInputFn fn, void* userdata) {
    vm->inputFn = fn;
    vm->inputUserdata = userdata;
}

static void defaultOutput(const std::string& s, UCLangVM* vm) {
    if (vm->outputFn) {
        vm->outputFn(s.c_str(), vm->outputUserdata);
    }
}

static std::string defaultInput(const std::string& prompt, UCLangVM* vm) {
    if (vm->inputFn) {
        const char* r = vm->inputFn(prompt.c_str(), vm->inputUserdata);
        return r ? r : "";
    }
    return "";
}

int uclang_vm_execute(UCLangVM* vm, const char* source, char** output) {
    try {
        // Capture output if requested
        std::stringstream capture;
        bool captureOutput = (output != nullptr);

        UCLang::Interpreter interp;
        interp.outputFn = [&](const std::string& s) {
            if (captureOutput) capture << s;
            defaultOutput(s, vm);
        };
        interp.inputFn = [&](const std::string& p) -> std::string {
            return defaultInput(p, vm);
        };
        interp.htmlFn = [](const std::string&) {};

        // Lex
        UCLang::Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // Parse
        UCLang::Parser parser(tokens);
        auto ast = parser.parse();

        // Execute
        interp.registerBuiltins();
        interp.execute(*ast);

        if (captureOutput) {
            std::string s = capture.str();
            *output = (char*)std::malloc(s.size() + 1);
            if (*output) { std::memcpy(*output, s.c_str(), s.size() + 1); }
        }
        return 0;
    } catch (const std::exception& e) {
        tls_error = e.what();
        vm->lastError = e.what();
        if (output) *output = nullptr;
        return 1;
    }
}

int uclang_vm_execute_file(UCLangVM* vm, const char* path, char** output) {
    std::ifstream file(path);
    if (!file.is_open()) {
        tls_error = "cannot open file";
        vm->lastError = "cannot open file";
        return 1;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return uclang_vm_execute(vm, buf.str().c_str(), output);
}

void uclang_free(char* ptr) {
    std::free(ptr);
}

const char* uclang_last_error(void) {
    return tls_error.c_str();
}

char* uclang_tokenize(const char* source) {
    try {
        UCLang::Lexer lexer(source);
        auto tokens = lexer.tokenize();

        auto esc = [](const std::string& s) -> std::string {
            std::string r;
            for (char c : s) {
                if (c == '"' || c == '\\') { r += '\\'; r += c; }
                else if (c >= 32) r += c;
                else { r += "\\u"; char buf[5]; std::sprintf(buf, "%04x", (unsigned char)c); r += buf; }
            }
            return r;
        };
        std::stringstream json;
        json << "[";
        for (size_t i = 0; i < tokens.size(); i++) {
            if (i > 0) json << ",";
            json << "{\"t\":" << (int)tokens[i].type
                 << ",\"v\":\"" << esc(tokens[i].value)
                 << "\",\"l\":" << tokens[i].line
                 << ",\"c\":" << tokens[i].column
                 << "}";
        }
        json << "]";
        std::string s = json.str();
        char* result = (char*)std::malloc(s.size() + 1);
        if (result) std::memcpy(result, s.c_str(), s.size() + 1);
        return result;
    } catch (...) {
        char* r = (char*)std::malloc(3);
        if (r) { r[0] = '['; r[1] = ']'; r[2] = 0; }
        return r;
    }
}
