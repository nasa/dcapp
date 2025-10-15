#include "file.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#endif

// get absolute path to exe
int dc_utils_get_exe_path(char *buffer, size_t size) {

    if (buffer == NULL || size == 0) {
        return -1;
    }

#if defined(_WIN32)
    GetModuleFileNameA(NULL, buffer, (DWORD)size);
#elif defined(__APPLE__)
    uint32_t bufsize = (uint32_t)size;
    if (_NSGetExecutablePath(buffer, &bufsize) != 0) {
        buffer[0] = '\0';
    } else {
        return 0;
    }
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return 0;
    } else {
        buffer[0] = '\0';
    }
#endif

    return -1;
}

int dc_utils_get_cwd(char *buffer, size_t size) {

    if (buffer == NULL || size == 0) {
        return -1;
    }

#if defined(_WIN32) || defined(_WIN64)
    if (_getcwd(buffer, (int)size) != NULL) {
#else
    if (getcwd(buffer, size) != NULL) {
#endif
        return 0;
    }
    perror("DCAPP get_current_working_directory()");
    return -1;
}

int dc_utils_join_paths(const char *dir, const char *rel_path, char *out, size_t out_size) {
    if (!dir || !rel_path || !out) {
        return -1;
    }

    size_t dir_len = strlen(dir);
    size_t rel_len = strlen(rel_path);

    if (dir_len + rel_len + 2 > out_size) {
        return -1;
    }

#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    bool needs_sep = (dir_len > 0 && dir[dir_len - 1] != '/' && dir[dir_len - 1] != '\\');

    if (needs_sep) {
        snprintf(out, out_size, "%s%c%s", dir, sep, rel_path);
    } else {
        snprintf(out, out_size, "%s%s", dir, rel_path);
    }

    return true;
}

bool dc_utils_is_relative_path(const char *path) {
    if (!path || !*path) {
        fprintf(stderr, "DCAPP dc_utils_is_relative_path(): invalid buffer\n");
        return false;
    }

#ifdef _WIN32
    if ((strlen(path) >= 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) || (path[0] == '\\' && path[1] == '\\')) {
#else
    if (path[0] == '/') {
#endif
        return false;
    }

    return true;
}

// check if path is absolute
bool dc_utils_is_absolute_path(const char *path) {
    if (!path || !*path) {
        fprintf(stderr, "DCAPP dc_utils_is_absolute_path(): invalid buffer\n");
        return false;
    }

#if defined(_WIN32)
    return ((strlen(path) > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) ||
            (path[0] == '\\' && path[1] == '\\'));
#else
    return path[0] == '/';
#endif
}

// check if path is canonical (no symbolic links, relatives)
bool dc_utils_is_canonical_path(const char *path) {
    if (!path || !*path) {
        fprintf(stderr, "DCAPP dc_utils_is_canonical_path(): invalid buffer\n");
        return false;
    }

#if defined(_WIN32)
    char  resolved[MAX_PATH];
    DWORD len = GetFullPathNameA(path, MAX_PATH, resolved, NULL);
    if (len == 0 || len >= MAX_PATH) {
        return 0;
    }
    return strcmp(path, resolved) == 0;
#else
    char *resolved = realpath(path, NULL);
    if (!resolved) {
        return false;
    }
    bool result = strcmp(path, resolved) == 0;
    free(resolved);
    return result;
#endif
}

int dc_utils_canonicalize_path(const char *path, char *out, size_t out_size) {

    if (path == NULL || out == NULL || out_size == 0) {
        fprintf(stderr, "DCAPP dc_utils_canonicalize_path(): invalid buffers\n");
        return -1;
    }

    char *fullpath;
#if defined(_WIN32) || defined(_WIN64)
    fullpath = _fullpath(NULL, path, 0);
#else
    fullpath = realpath(path, NULL);
#endif

    if (!fullpath) {
        fprintf(stderr, "DCAPP dc_utils_canonicalize_path(): invalid path returned from syscall: %s\n", path);
        return -1;
    } else if (strlen(fullpath) >= out_size) {
        fprintf(stderr, "DCAPP dc_utils_canonicalize_path(): canonical path too long for buffer\n");
        return -1;
    }

    strcpy(out, fullpath);
    free(fullpath);
    return 0;
}

int dc_utils_get_directory(const char *path, char *out, size_t out_size) {
    if (!path || !out || out_size == 0) {
        fprintf(stderr, "DCAPP dc_utils_get_directory(): invalid buffers\n");
        return -1;
    }

    const char *last_slash = strrchr(path, '/');
#if defined(_WIN32)
    const char *last_backslash = strrchr(path, '\\');
    if (!last_slash || (last_backslash && last_backslash > last_slash)) {
        last_slash = last_backslash;
    }
#endif

    if (last_slash) {
        size_t len = last_slash - path;
        if (len >= out_size)
            len = out_size - 1;
        strncpy(out, path, len);
        out[len] = '\0';
    } else {
        // No directory separator found; assume current directory
        strncpy(out, ".", out_size - 1);
        out[out_size - 1] = '\0';
    }
    return 0;
}

int dc_utils_create_directory(const char *path) {
    if (!path) {
        fprintf(stderr, "DCAPP dc_utils_create_directory(): invalid buffer\n");
        return -1;
    }

#if defined(_WIN32)
    if (_mkdir(path) == 0) {
        return 0;
    }
#else
    if (mkdir(path, 0755) == 0) {
        return 0;
    }
#endif

#if defined(_WIN32) || defined(_WIN64)
    if (errno == EEXIST)
        return 0;
#else
    if (errno == EEXIST)
        return 0;
#endif

    perror("DCAPP dc_utils_create_directory()");
    return -1;
}
