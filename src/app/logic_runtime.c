#include "logic_runtime.h"
#include "logic_callbacks.h"
#include "value.h"

#include "pl.h"

#include "utils/file.h"
#include "utils/library.h"
#include "utils/log.h"
#include "utils/time.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define DC_APP_MAX_LOGIC_FRAME_DELTA       0.25
#define DC_APP_MAX_LOGIC_UPDATES_PER_FRAME 16

struct DcAppLogicContext {
    DcLibrary *library;
    DcAppLogicPreInitFn pre_init;
    DcAppLogicInitFn initialize;
    DcAppLogicUpdateFn update;
    DcAppLogicCloseFn close;
    void *user_data;

    double last_update_time;
    double update_accumulator;
    double last_update_rate;
};

static const plMemoryI *_ext_memory = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_logic_runtime_init(plApiRegistryI *api_registry) {
    _ext_memory = pl_get_api_latest(api_registry, plMemoryI);
}

DcAppLogicContext *dc_app_logic_context_create(void) {
    DcAppLogicContext *logic = PL_ALLOC(sizeof(*logic));
    if (!logic) return NULL;
    memset(logic, 0, sizeof(*logic));
    return logic;
}

void dc_app_logic_context_destroy(DcAppLogicContext *logic, struct DcAppContext *app_context) {
    if (!logic) return;

    // call logic close
    // User cleanup must run before unloading the library that implements it.
    if (logic->close) {
        logic->close(app_context, logic->user_data);
    }

    // unload logic shared library
    if (logic->library) {
        dc_utils_library_close(logic->library);
    }
    PL_FREE(logic);
}

bool dc_app_logic_load(DcAppLogicContext *logic, const char *path, const char *base_directory) {
    if (!logic || !path || path[0] == '\0') return false;
    if (logic->library) {
        DC_LOG_ERROR("Logic", "Duplicate <Logic> definitions");
        return false;
    }

    // convert to absolute path
    char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
    if (dc_utils_is_relative_path(path)) {
        dc_utils_join_paths(base_directory, path, abs_filepath, sizeof(abs_filepath));
    } else {
        strcpy(abs_filepath, path);
    }

    // strip any user-supplied platform extension; try each in turn
    char base_filepath[DC_VALUE_STRING_BUFFER_SIZE];
    strncpy(base_filepath, abs_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
    base_filepath[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';

    char *ext = strrchr(base_filepath, '.');
    if (ext && (strcmp(ext, ".so") == 0 || strcmp(ext, ".dylib") == 0 || strcmp(ext, ".dll") == 0)) {
        *ext = '\0';
    }

    char lib_base_filepath[DC_VALUE_STRING_BUFFER_SIZE];
    char *slash     = strrchr(base_filepath, '/');
    char *backslash = strrchr(base_filepath, '\\');
    char *separator = slash;
    if (backslash && (!separator || backslash > separator)) {
        separator = backslash;
    }
    if (separator) {
        int prefix_length = (int)(separator - base_filepath + 1);
        snprintf(lib_base_filepath, sizeof(lib_base_filepath), "%.*slib%s", prefix_length, base_filepath, separator + 1);
    } else {
        snprintf(lib_base_filepath, sizeof(lib_base_filepath), "lib%s", base_filepath);
    }

    const char *base_filepaths[] = {base_filepath, lib_base_filepath};
    const char *extensions[]     = {".so", ".dylib", ".dll"};
    char        try_filepath[DC_VALUE_STRING_BUFFER_SIZE];

    for (int base_index = 0; base_index < 2 && !logic->library; base_index++) {
        for (int ext_index = 0; ext_index < 3 && !logic->library; ext_index++) {
            snprintf(try_filepath, sizeof(try_filepath), "%s%s", base_filepaths[base_index], extensions[ext_index]);
            if (!dc_utils_file_exists(try_filepath)) continue;
            logic->library = dc_utils_library_load(try_filepath);
        }
    }

    if (!logic->library) {
        DC_LOG_ERROR("Logic", "Failed to load library '%s.so/.dylib/.dll': %s", base_filepath, dc_utils_library_last_error());
        return false;
    }

    // These fixed names form the lifecycle ABI between dcapp and the logic library.
    logic->pre_init   = (DcAppLogicPreInitFn)dc_utils_library_symbol(logic->library, "display_pre_init");
    logic->initialize = (DcAppLogicInitFn)dc_utils_library_symbol(logic->library, "display_init");
    logic->update     = (DcAppLogicUpdateFn)dc_utils_library_symbol(logic->library, "display_draw");
    logic->close      = (DcAppLogicCloseFn)dc_utils_library_symbol(logic->library, "display_close");
    return true;
}

bool dc_app_logic_is_loaded(const DcAppLogicContext *logic) {
    return logic && logic->library;
}

void *dc_app_logic_symbol(DcAppLogicContext *logic, const char *name) {
    return logic && logic->library && name ? dc_utils_library_symbol(logic->library, name) : NULL;
}

void dc_app_logic_pre_init(DcAppLogicContext *logic, const struct DcAppInit *init) {
    if (logic && logic->pre_init) {
        logic->pre_init(init);
    }
}

void dc_app_logic_initialize(DcAppLogicContext *logic, struct DcAppContext *app_context) {
    if (logic && logic->initialize) {
        logic->initialize(app_context, &logic->user_data);
    }
}

void dc_app_logic_update(DcAppLogicContext *logic, struct DcAppContext *app_context, double update_rate) {
    if (!logic || !logic->update) return;

    if (!isfinite(update_rate) || update_rate <= 0.0) {
        logic->last_update_time   = dc_utils_time_get();
        logic->update_accumulator = 0.0;
        logic->last_update_rate   = 0.0;
        logic->update(app_context, logic->user_data);
        return;
    }

    double update_interval = 1.0 / update_rate;
    double now             = dc_utils_time_get();

    if (logic->last_update_time <= 0.0 || logic->last_update_rate != update_rate) {
        logic->last_update_time   = now;
        logic->update_accumulator = update_interval;
        logic->last_update_rate   = update_rate;
    } else {
        double elapsed = now - logic->last_update_time;
        if (elapsed > DC_APP_MAX_LOGIC_FRAME_DELTA) {
            elapsed = DC_APP_MAX_LOGIC_FRAME_DELTA;
        }
        logic->last_update_time = now;
        logic->update_accumulator += elapsed;
    }

    // Catch up fixed-rate updates without allowing one slow frame to cause unbounded work.
    int update_count = 0;
    while (logic->update_accumulator >= update_interval &&
           update_count < DC_APP_MAX_LOGIC_UPDATES_PER_FRAME) {
        logic->update(app_context, logic->user_data);
        logic->update_accumulator -= update_interval;
        update_count++;
    }

    if (update_count == DC_APP_MAX_LOGIC_UPDATES_PER_FRAME &&
        logic->update_accumulator >= update_interval) {
        // Drop excess backlog so a stalled frame cannot leave logic permanently behind.
        logic->update_accumulator = 0.0;
    }
}

void *dc_app_logic_user_data(DcAppLogicContext *logic) {
    return logic ? logic->user_data : NULL;
}
