#ifndef _DC_UTILS_FILE_
#define _DC_UTILS_FILE_

#include <stdbool.h>
#include <stddef.h>

static const int DC_UTILS_FILEPATH_BUFFER_SIZE = 4096;

#ifdef __cplusplus
extern "C" {
#endif

void dc_utils_get_executable_path(char *buffer, size_t size);
bool dc_utils_join_paths(const char *dir, const char *rel_path, char *out, size_t out_size);
int  dc_utils_is_relative_path(const char *path);
int  dc_utils_is_absolute_path(const char *path);
int  dc_utils_is_canonical_path(const char *path);

#ifdef __cplusplus
}
#endif

#endif
