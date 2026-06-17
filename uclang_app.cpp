#include "core.h"
#include <windows.h>
#include <string>

const char* g_embeddedScript = R"uc_embed(main(.
  Print("Hello").
  n == 42.
  Print(n).
  result == run.identity(n.).
  Print("got:").
  Print(result).
).

identity(x.
  response(x).
).

run.main(.).
)uc_embed";

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    UCLangVM* vm = uclang_vm_new();
    char* output = nullptr;
    int ret = uclang_vm_execute(vm, g_embeddedScript, &output);
    std::string msg;
    if (output) { msg = output; uclang_free(output); }
    if (ret) { if (!msg.empty()) msg += "\r\n"; msg += "Error: "; msg += uclang_last_error(); }
    if (!msg.empty()) MessageBoxA(nullptr, msg.c_str(), "UCLang App", MB_OK);
    uclang_vm_free(vm);
    return ret;
}

