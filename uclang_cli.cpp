#include "core.h"
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: uclang <file.uc> [args...]\n");
        return 1;
    }

    UCLangVM* vm = uclang_vm_new();
    if (!vm) { fprintf(stderr, "Failed to create VM\n"); return 1; }

    char* output = nullptr;
    int ret = uclang_vm_execute_file(vm, argv[1], &output);

    if (output) {
        printf("%s", output);
        uclang_free(output);
    }

    if (ret) {
        fprintf(stderr, "\nError: %s\n", uclang_last_error());
    }

    uclang_vm_free(vm);
    return ret;
}
