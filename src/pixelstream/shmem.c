#ifndef _WIN32

#include "shmem.h"
#include "../utils/stb_sb.h"
#include "../utils/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>

// Shared memory structure - must match writer's layout
typedef struct {
    uint32_t writing;         // writer is currently writing
    uint32_t reading;         // reader is currently reading
    uint64_t buffercount;     // incremented each write
    uint32_t width;           // frame width
    uint32_t height;          // frame height
    uint32_t bufferrequested; // reader requests new buffer
} _ShmemHeader;

#define _SHM_HEADER_SIZE 1024 // safe size for shmget

struct DcPsShmemSource {
    DcPsShmemContext *context;

    // config
    char *filepath;

    // shared memory
    _ShmemHeader *shm;

    // state
    bool connected;
    bool has_new_data;
    uint64_t buffercount;

    // reconnect
    uint32_t stale_frames;

    // latest frame
    unsigned char *pixels;
    uint32_t width;
    uint32_t height;
    size_t alloc_size;
};

struct DcPsShmemContext {
    // Sources are separate allocations so registry growth cannot invalidate their addresses.
    DcPsShmemSource **sb_sources;
};

#define _MAX_SOURCES 10
#define _STALE_THRESHOLD 300 // frames without new data before reconnect (~5s at 60fps)

// static functions
static int _try_attach_shm(DcPsShmemSource *ctx);
static int _read_frame(DcPsShmemSource *ctx);
static void _detach_shm(DcPsShmemSource *ctx);
static void _source_cleanup(DcPsShmemSource *ctx);

DcPsShmemContext *dc_ps_shmem_context_create(void) {
    DcPsShmemContext *context = calloc(1, sizeof(DcPsShmemContext));
    if (!context) return NULL;

    sbgrow(context->sb_sources, _MAX_SOURCES, sizeof(*context->sb_sources));
    return context;
}

void dc_ps_shmem_update(DcPsShmemContext *context) {
    if (!context) return;

    for (int ii = 0; ii < sbcount(context->sb_sources); ii++) {
        DcPsShmemSource *ctx = context->sb_sources[ii];
        if (!ctx) continue;

        ctx->has_new_data = false;

        // try to attach if not yet connected
        if (!ctx->shm) {
            _try_attach_shm(ctx);
        }

        // read if connected
        if (ctx->shm) {
            _read_frame(ctx);
        }
    }
}

void dc_ps_shmem_context_destroy(DcPsShmemContext *context) {
    if (!context) return;

    for (int ii = 0; ii < sbcount(context->sb_sources); ii++) {
        DcPsShmemSource *ctx = context->sb_sources[ii];
        if (!ctx) continue;
        _source_cleanup(ctx);
        free(ctx);
    }
    sbfree(context->sb_sources);
    free(context);
}

DcPsShmemSource *dc_ps_shmem_add_source(DcPsShmemContext *context, const char *filepath) {
    if (!context) return NULL;

    DcPsShmemSource *ctx = (DcPsShmemSource *)malloc(sizeof(DcPsShmemSource));
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof(DcPsShmemSource));

    ctx->filepath = strdup(filepath);
    if (!ctx->filepath) {
        DC_LOG_ERROR("Shmem", "Failed to allocate filepath");
        free(ctx);
        return NULL;
    }
    ctx->shm = NULL;
    ctx->connected = false;
    ctx->has_new_data = false;
    ctx->buffercount = 0;
    ctx->pixels = NULL;
    ctx->width = 0;
    ctx->height = 0;
    ctx->alloc_size = 0;
    ctx->context = context;

    bool stored = false;
    for (int ii = 0; ii < sbcount(context->sb_sources); ii++) {
        if (!context->sb_sources[ii]) {
            context->sb_sources[ii] = ctx;
            stored = true;
            break;
        }
    }
    if (!stored)
        sbpush(context->sb_sources, ctx);

    return ctx;
}

void dc_ps_shmem_remove_source(DcPsShmemSource *source) {
    DcPsShmemSource *ctx = source;
    if (!ctx) return;

    DcPsShmemContext *context = ctx->context;
    if (context) {
        for (int ii = 0; ii < sbcount(context->sb_sources); ii++) {
            if (context->sb_sources[ii] == ctx) {
                context->sb_sources[ii] = NULL;
                break;
            }
        }
    }

    _source_cleanup(ctx);
    free(ctx);
}

bool dc_ps_shmem_is_connected(DcPsShmemSource *source) {
    return source && source->connected;
}

bool dc_ps_shmem_has_new_data(DcPsShmemSource *source) {
    return source && source->has_new_data;
}

void dc_ps_shmem_get_data(DcPsShmemSource *source, unsigned char *out_data, size_t out_data_size, size_t *out_size) {
    DcPsShmemSource *ctx = source;
    if (!ctx) {
        *out_size = 0;
        return;
    }

    size_t frame_size = (size_t)ctx->width * ctx->height * 4; // RGBA

    if (out_data_size < frame_size) {
        DC_LOG_ERROR("Shmem", "dc_ps_shmem_get_data(): output buffer too small");
        *out_size = 0;
        return;
    }

    if (ctx->pixels && frame_size > 0) {
        memcpy(out_data, ctx->pixels, frame_size);
        *out_size = frame_size;
    } else {
        *out_size = 0;
    }
}

uint32_t dc_ps_shmem_get_width(DcPsShmemSource *source) {
    return source ? source->width : 0;
}

uint32_t dc_ps_shmem_get_height(DcPsShmemSource *source) {
    return source ? source->height : 0;
}

// ----------------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------------

static int _try_attach_shm(DcPsShmemSource *ctx) {
    // Check if file exists
    FILE *fp = fopen(ctx->filepath, "r");
    if (!fp) {
        return -1; // file doesn't exist yet
    }
    fclose(fp);

    // Generate shared memory key from filepath
    key_t key = ftok(ctx->filepath, 'R');
    if (key == -1) {
        DC_LOG_ERROR("Shmem", "ftok failed: %s", strerror(errno));
        return -1;
    }

    // Get shared memory segment
    int shmid = shmget(key, _SHM_HEADER_SIZE, IPC_CREAT | 0777);
    if (shmid < 0) {
        DC_LOG_ERROR("Shmem", "shmget failed: %s", strerror(errno));
        return -1;
    }

    // Attach to shared memory
    ctx->shm = (_ShmemHeader *)shmat(shmid, NULL, 0);
    if (ctx->shm == (void *)-1) {
        DC_LOG_ERROR("Shmem", "shmat failed: %s", strerror(errno));
        ctx->shm = NULL;
        return -1;
    }

    return 0;
}

static void _detach_shm(DcPsShmemSource *ctx) {
    if (ctx->shm) {
        shmdt(ctx->shm);
        ctx->shm = NULL;
    }
    ctx->connected = false;
    ctx->buffercount = 0;
    ctx->stale_frames = 0;
}

static int _read_frame(DcPsShmemSource *ctx) {
    uint32_t on = 1, off = 0;

    ctx->connected = true;

    // Don't read while writer is writing
    if (ctx->shm->writing) {
        return 0;
    }

    // Signal that we want data and are reading
    memcpy(&ctx->shm->bufferrequested, &on, 4);
    memcpy(&ctx->shm->reading, &on, 4);

    // Check if new frame available
    if (ctx->buffercount != ctx->shm->buffercount) {
        ctx->buffercount = ctx->shm->buffercount;
        ctx->stale_frames = 0;
        ctx->width = ctx->shm->width;
        ctx->height = ctx->shm->height;

        // Read pixel data from file
        FILE *fp = fopen(ctx->filepath, "r");
        if (fp) {
            size_t nbytes = (size_t)ctx->width * ctx->height * 4; // RGBA

            // Reallocate if needed
            if (nbytes > ctx->alloc_size) {
                void *new_pixels = realloc(ctx->pixels, nbytes);
                if (!new_pixels) {
                    DC_LOG_ERROR("Shmem", "Failed to reallocate pixel buffer");
                    fclose(fp);
                    memcpy(&ctx->shm->reading, &off, 4);
                    return 0;
                }
                ctx->pixels = new_pixels;
                ctx->alloc_size = nbytes;
            }

            rewind(fp);
            fread(ctx->pixels, 1, nbytes, fp);
            fclose(fp);

            ctx->has_new_data = true;
        }
    } else {
        ctx->stale_frames++;
        if (ctx->stale_frames >= _STALE_THRESHOLD) {
            DC_LOG_WARN("Shmem", "Stream '%s' stale, reconnecting", ctx->filepath);
            memcpy(&ctx->shm->reading, &off, 4);
            _detach_shm(ctx);
            return 0;
        }
    }

    // Signal done reading
    memcpy(&ctx->shm->reading, &off, 4);

    return ctx->has_new_data ? 1 : 0;
}

static void _source_cleanup(DcPsShmemSource *ctx) {
    _detach_shm(ctx);
    free(ctx->filepath);
    free(ctx->pixels);
}

#else

typedef int _dc_ps_shmem_c_unused; // !_WIN32

#endif // !_WIN32
