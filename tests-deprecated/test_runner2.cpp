#include <cstdio>
#include "core.h"

int main() {
    UCLangVM* vm = uclang_vm_new();
    char* out = nullptr;
    int ret = uclang_vm_execute_file(vm, "test.uc", &out);
    if (ret) printf("\nError: %s\n", uclang_last_error());
    if (out) { printf("%s", out); uclang_free(out); }
    else printf("(no output)");
    uclang_vm_free(vm);
    return ret;
}
