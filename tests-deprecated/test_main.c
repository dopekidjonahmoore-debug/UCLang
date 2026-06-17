#include "src/ucl_runtime.h"
int main(void) {
    Env _env = {0};
    env_set(&_env, "_tmpc", val_int(0));
    env_set(&_env, "_curenv", val_string("&_env"));
    ucl_print(val_string("start"));
    
    Value _a426[1];
    _a426[0] = val_string("test");
    env_set(&_env, "len", builtin_str_len(1, _a426));
    
    Value _a427[1];
    _a427[0] = env_get(&_env, "len");
    ucl_print(val_add(val_string("len = "), builtin_int_to_str(1, _a427)));
    
    return 0;
}
