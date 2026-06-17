#include "src/ucl_runtime.h"
#include <stdio.h>
int main(void) {
    printf("A\n"); fflush(stdout);
    Env _env = {0};
    printf("B\n"); fflush(stdout);
    env_set(&_env, "_tmpc", val_int(0));
    printf("C\n"); fflush(stdout);
    ucl_print(val_string("start"));
    printf("D\n"); fflush(stdout);
    return 0;
}
