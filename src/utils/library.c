#include "library.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#define _DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE 512

struct _DcLibrary {
#ifdef _WIN32
    HMODULE handle;
#else
    void *handle;
#endif
};

static char _dc_utils_library_last_error_buffer[_DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE];

static void _dc_utils_library_capture_error(const char *fallback) {
#ifdef _WIN32
    DWORD err = GetLastError();
    if (err == 0) {
        strncpy(_dc_utils_library_last_error_buffer,
                fallback ? fallback : "unknown error",
                _DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE - 1);
        _dc_utils_library_last_error_buffer[_DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE - 1] = '\0';
        return;
    }
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   _dc_utils_library_last_error_buffer,
                   _DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE - 1, NULL);
    _dc_utils_library_last_error_buffer[_DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE - 1] = '\0';
#else
    const char *err = dlerror();
    strncpy(_dc_utils_library_last_error_buffer,
            err ? err : (fallback ? fallback : "unknown error"),
            _DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE - 1);
    _dc_utils_library_last_error_buffer[_DC_UTILS_LIBRARY_ERROR_BUFFER_SIZE - 1] = '\0';
#endif
}

DcLibrary *dc_utils_library_load(const char *path) {
    if (!path || !path[0]) {
        _dc_utils_library_capture_error("empty path");
        return NULL;
    }

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path);
#else
    dlerror();
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif

    if (!handle) {
        _dc_utils_library_capture_error("load failed");
        return NULL;
    }

    DcLibrary *lib = (DcLibrary *)malloc(sizeof(DcLibrary));
    if (!lib) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        _dc_utils_library_capture_error("allocation failed");
        return NULL;
    }
    lib->handle = handle;
    return lib;
}

void *dc_utils_library_symbol(DcLibrary *lib, const char *name) {
    if (!lib || !lib->handle || !name) {
        _dc_utils_library_capture_error("invalid library or symbol name");
        return NULL;
    }

#ifdef _WIN32
    FARPROC sym = GetProcAddress(lib->handle, name);
    if (!sym) _dc_utils_library_capture_error("symbol not found");
    return (void *)sym;
#else
    dlerror();
    void *sym = dlsym(lib->handle, name);
    if (!sym) _dc_utils_library_capture_error("symbol not found");
    return sym;
#endif
}

void dc_utils_library_close(DcLibrary *lib) {
    if (!lib) return;
    if (lib->handle) {
#ifdef _WIN32
        FreeLibrary(lib->handle);
#else
        dlclose(lib->handle);
#endif
    }
    free(lib);
}

const char *dc_utils_library_last_error(void) {
    return _dc_utils_library_last_error_buffer;
}
