// dcapp includes
#include "file.h"

// c standard includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// platform includes
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

// get absolute path to exe
void dc_utils_get_executable_path(char *buffer, size_t size) {
#if defined(_WIN32)
    GetModuleFileNameA(NULL, buffer, (DWORD)size);
#elif defined(__APPLE__)
    uint32_t bufsize = (uint32_t)size;
    if (_NSGetExecutablePath(buffer, &bufsize) != 0) {
        buffer[0] = '\0';
    }
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
    } else {
        buffer[0] = '\0';
    }
#endif
}

bool dc_utils_join_paths(const char *dir, const char *rel_path, char *out, size_t out_size) {
    if (!dir || !rel_path || !out)
        return false;

    size_t dir_len = strlen(dir);
    size_t rel_len = strlen(rel_path);

    if (dir_len + rel_len + 2 > out_size)
        return false;

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

bool is_relative_path(const char *path) {
    if (path == NULL || path[0] == '\0') {
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
int dc_utils_is_absolute_path(const char *path) {
    if (!path || !*path)
        return 0;

#if defined(_WIN32)
    return ((strlen(path) > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) ||
            (path[0] == '\\' && path[1] == '\\'));
#else
    return path[0] == '/';
#endif
}

// check if path is canonical (no symbolic links, relatives)
int is_canonical_path(const char *path) {
#if defined(_WIN32)
    char  resolved[MAX_PATH];
    DWORD len = GetFullPathNameA(path, MAX_PATH, resolved, NULL);
    if (len == 0 || len >= MAX_PATH)
        return 0;
    return strcmp(path, resolved) == 0;
#else
    char *resolved = realpath(path, NULL);
    if (!resolved) {
        return 0;
    }
    int result = strcmp(path, resolved) == 0;
    free(resolved);
    return result;
#endif
}
