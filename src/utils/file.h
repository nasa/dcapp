#ifndef _DC_UTILS_FILE_
#define _DC_UTILS_FILE_

#include <stdbool.h>
#include <stddef.h>

#define DC_UTILS_FILEPATH_BUFFER_SIZE 4096

#ifdef __cplusplus
extern "C" {
#endif

int            dc_utils_get_exe_path(char *buffer, size_t size);
int            dc_utils_get_cwd(char *buffer, size_t size);
int            dc_utils_join_paths(const char *dir, const char *rel_path, char *out, size_t out_size);
bool           dc_utils_is_relative_path(const char *path);
bool           dc_utils_is_absolute_path(const char *path);
bool           dc_utils_is_canonical_path(const char *path);
int            dc_utils_canonicalize_path(const char *path, char *out, size_t out_size);
int            dc_utils_get_directory(const char *path, char *out, size_t out_size);
int            dc_utils_create_directory(const char *path);
unsigned char *dc_utils_load_binary_file(const char *path, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
