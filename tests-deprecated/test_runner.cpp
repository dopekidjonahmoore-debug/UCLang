#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include "core.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"

static std::string readFile(const char* path) {
    std::ifstream f(path);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--compile") {
        const char* srcPath = (argc > 2) ? argv[2] : "test.uc";
        std::string src = readFile(srcPath);
        if (src.empty()) { fprintf(stderr, "Cannot read %s\n", srcPath); return 1; }

        UCLang::Lexer lexer(src);
        auto tokens = lexer.tokenize();
        UCLang::Parser parser(tokens);
        auto ast = parser.parse();

        UCLang::Compiler compiler;
        std::string cCode = compiler.compile(*ast);

        std::string outPath = std::string(srcPath) + ".c";
        if (outPath == "test.uc.c") outPath = "test.c";
        std::ofstream out(outPath);
        if (!out) { fprintf(stderr, "Cannot write %s\n", outPath.c_str()); return 1; }
        out << cCode;
        out.close();

        printf("Compiled %s -> %s (%zu bytes)\n", srcPath, outPath.c_str(), cCode.size());
        return 0;
    }

    UCLangVM* vm = uclang_vm_new();
    char* out = nullptr;
    int ret = uclang_vm_execute_file(vm, "test.uc", &out);
    if (ret) {
        printf("Error: %s\n", uclang_last_error());
    }
    if (out) {
        printf("%s", out);
        uclang_free(out);
    } else {
        printf("(no output)\n");
    }
    uclang_vm_free(vm);
    printf("\n--- exit %d ---\n", ret);
    return ret;
}
