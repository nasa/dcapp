#include "env.h"

#include <stdlib.h>
#include <string.h>

const char *dc_utils_get_env(const char *name) {
    const char *val = getenv(name);
    return (val != NULL) ? val : "";
}

int dc_utils_set_env(const char *name, const char *value, int overwrite) {
#if defined(_WIN32)
    if (!overwrite) {
        char *existing = getenv(name);
        if (existing != NULL) {
            return 0; // don't overwrite
        }
    }
    return _putenv_s(name, value);
#else
    return setenv(name, value, overwrite);
#endif
}
