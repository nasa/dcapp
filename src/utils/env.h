#ifndef _DC_UTILS_ENV_
#define _DC_UTILS_ENV_

#ifdef __cplusplus
extern "C" {
#endif

const char *dc_utils_get_env(const char *name);
int         dc_utils_set_env(const char *name, const char *value, int overwrite);

#ifdef __cplusplus
}
#endif

#endif
