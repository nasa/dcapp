#ifndef _DC_PIXELSTREAM_SHMEM_
#define _DC_PIXELSTREAM_SHMEM_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct DcPsShmemContext DcPsShmemContext;
typedef struct DcPsShmemSource  DcPsShmemSource;

#ifndef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

DcPsShmemContext *dc_ps_shmem_context_create(void);
void              dc_ps_shmem_context_destroy(DcPsShmemContext *context);
void              dc_ps_shmem_update(DcPsShmemContext *context);

// individual sources
DcPsShmemSource *dc_ps_shmem_add_source(DcPsShmemContext *context, const char *filepath);
void             dc_ps_shmem_remove_source(DcPsShmemSource *source);
bool             dc_ps_shmem_is_connected(DcPsShmemSource *source);
bool             dc_ps_shmem_has_new_data(DcPsShmemSource *source);
void             dc_ps_shmem_get_data(DcPsShmemSource *source, unsigned char *out_data, size_t out_data_size, size_t *out_size);
uint32_t         dc_ps_shmem_get_width(DcPsShmemSource *source);
uint32_t         dc_ps_shmem_get_height(DcPsShmemSource *source);

#ifdef __cplusplus
}
#endif

#else

// Shared memory pixel streams are not supported on Windows.
static inline DcPsShmemContext *dc_ps_shmem_context_create(void) {
    return NULL;
}

static inline void dc_ps_shmem_context_destroy(DcPsShmemContext *context) {
    (void)context;
}

static inline void dc_ps_shmem_update(DcPsShmemContext *context) {
    (void)context;
}

static inline DcPsShmemSource *dc_ps_shmem_add_source(DcPsShmemContext *context, const char *filepath) {
    (void)context;
    (void)filepath;
    return NULL;
}

static inline void dc_ps_shmem_remove_source(DcPsShmemSource *source) {
    (void)source;
}

static inline bool dc_ps_shmem_is_connected(DcPsShmemSource *source) {
    (void)source;
    return false;
}

static inline bool dc_ps_shmem_has_new_data(DcPsShmemSource *source) {
    (void)source;
    return false;
}

static inline void dc_ps_shmem_get_data(DcPsShmemSource *source, unsigned char *out_data, size_t out_data_size, size_t *out_size) {
    (void)source;
    (void)out_data;
    (void)out_data_size;
    *out_size = 0;
}

static inline uint32_t dc_ps_shmem_get_width(DcPsShmemSource *source) {
    (void)source;
    return 0;
}

static inline uint32_t dc_ps_shmem_get_height(DcPsShmemSource *source) {
    (void)source;
    return 0;
}

#endif // _WIN32

#endif
