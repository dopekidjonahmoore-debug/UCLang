#include "core.h"
#include <windows.h>
#include <string>

// File variable declarations will be placed here by the export script
// FILE_CONTENTS

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::string fullScript;
    // File concatenation code will be placed here by the export script
    // FILE_CONCAT

    UCLangVM* vm = uclang_vm_new();
    char* output = nullptr;
    int ret = uclang_vm_execute(vm, fullScript.c_str(), &output);
    std::string msg;
    if (output) { msg = output; uclang_free(output); }
    if (ret) { if (!msg.empty()) msg += "\r\n"; msg += "Error: "; msg += uclang_last_error(); }
    if (!msg.empty()) MessageBoxA(nullptr, msg.c_str(), "UCLang App", MB_OK);
    uclang_vm_free(vm);
    return ret;
}
