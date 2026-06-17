#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCLangVM UCLangVM;

// Callbacks
typedef void (*UCLangOutputFn)(const char* text, void* userdata);
typedef const char* (*UCLangInputFn)(const char* prompt, void* userdata);

// Create/destroy a VM instance
UCLangVM* uclang_vm_new(void);
void      uclang_vm_free(UCLangVM* vm);

// Set I/O callbacks (default: stdout/stdin)
void uclang_vm_set_output(UCLangVM* vm, UCLangOutputFn fn, void* userdata);
void uclang_vm_set_input(UCLangVM* vm, UCLangInputFn fn, void* userdata);

// Execute UCLang source. Returns 0 on success, non-zero on error.
// If output is non-null, it receives the captured output (must be freed with uclang_free).
int  uclang_vm_execute(UCLangVM* vm, const char* source, char** output);

// Execute a .uc file. Convenience wrapper.
int  uclang_vm_execute_file(UCLangVM* vm, const char* path, char** output);

// Free string allocated by the library
void uclang_free(char* ptr);

// Get last error message (thread-local)
const char* uclang_last_error(void);

// Tokenize and return JSON-formatted token list (for IDE highlighting)
// Caller must free with uclang_free.
char* uclang_tokenize(const char* source);

#ifdef __cplusplus
}
#endif
