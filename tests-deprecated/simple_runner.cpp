#include <cstdio>
#include "core.h"

int main() {
    UCLangVM* vm = uclang_vm_new();
    char* out = nullptr;
    int ret = uclang_vm_execute_file(vm, "simple.uc", &out);
    if (ret) printf("Error: %s\n", uclang_last_error());
    if (out) { printf("%s", out); uclang_free(out); }
    else printf("(no output)\n");
    uclang_vm_free(vm);
    printf("\n--- exit %d ---\n", ret);
    return ret;
}
