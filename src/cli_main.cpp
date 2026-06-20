#include <cstdio>
#include <cstdlib>
#include <string>
#include "core.h"
#include <SDL.h>

#undef main

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: uclang <file.uc>\n");
        return 1;
    }
    UCLangVM* vm = uclang_vm_new();
    char* out = nullptr;
    int ret = uclang_vm_execute_file(vm, argv[1], &out);
    if (out) { fprintf(stdout, "%s", out); uclang_free(out); }
    if (ret) { fprintf(stderr, "Error: %s\n", uclang_last_error()); }
    uclang_vm_free(vm);
    return ret;
}
