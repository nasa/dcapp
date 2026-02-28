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

typedef struct {
    // config
    char *filepath;

    // shared memory
    _ShmemHeader *shm;

    // state
    bool     connected;
    bool     has_new_data;
    uint64_t buffercount;

    // latest frame
    unsigned char *pixels;
    uint32_t       width;
    uint32_t       height;
    size_t         alloc_size;

} _Context;

#define _MAX_SOURCES 10

// static vars
static _Context *_sb_contexts = NULL;

// static functions
static int _try_attach_shm(_Context *ctx);
static int _read_frame(_Context *ctx);

void dc_ps_shmem_init(void) {
    sbgrow(_sb_contexts, _MAX_SOURCES, sizeof(*_sb_contexts));
}

void dc_ps_shmem_update(void) {
    for (int ii = 0; ii < sbcount(_sb_contexts); ii++) {
        _Context *ctx = &_sb_contexts[ii];

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

void dc_ps_shmem_cleanup(void) {
    for (int ii = 0; ii < sbcount(_sb_contexts); ii++) {
        _Context *ctx = &_sb_contexts[ii];

        if (ctx->shm) {
            shmdt(ctx->shm);
        }
        free(ctx->filepath);
        free(ctx->pixels);
    }
    sbfree(_sb_contexts);
    _sb_contexts = NULL;
}

DcPsShmemHandle dc_ps_shmem_add_source(const char *filepath) {
    _Context ctx = {0};

    ctx.filepath = strdup(filepath);
    if (!ctx.filepath) {
        DC_LOG_ERROR("Shmem", "Failed to allocate filepath");
        DcPsShmemHandle handle = {0};
        return handle;
    }
    ctx.shm          = NULL;
    ctx.connected    = false;
    ctx.has_new_data = false;
    ctx.buffercount  = 0;
    ctx.pixels       = NULL;
    ctx.width        = 0;
    ctx.height       = 0;
    ctx.alloc_size   = 0;

    sbpush(_sb_contexts, ctx);

    DcPsShmemHandle handle = {0};
    handle._index          = (uint8_t)(sbcount(_sb_contexts) - 1);
    return handle;
}

void dc_ps_shmem_remove_source(DcPsShmemHandle handle) {
    _Context *ctx = &_sb_contexts[handle._index];

    if (ctx->shm) {
        shmdt(ctx->shm);
        ctx->shm = NULL;
    }

    free(ctx->pixels);
    ctx->pixels       = NULL;
    ctx->connected    = false;
    ctx->has_new_data = false;
}

bool dc_ps_shmem_is_connected(DcPsShmemHandle handle) {
    return _sb_contexts[handle._index].connected;
}

bool dc_ps_shmem_has_new_data(DcPsShmemHandle handle) {
    return _sb_contexts[handle._index].has_new_data;
}

void dc_ps_shmem_get_data(DcPsShmemHandle handle, unsigned char *out_data, size_t out_data_size, size_t *out_size) {
    _Context *ctx = &_sb_contexts[handle._index];

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

uint32_t dc_ps_shmem_get_width(DcPsShmemHandle handle) {
    return _sb_contexts[handle._index].width;
}

uint32_t dc_ps_shmem_get_height(DcPsShmemHandle handle) {
    return _sb_contexts[handle._index].height;
}

// ----------------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------------

static int _try_attach_shm(_Context *ctx) {
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

static int _read_frame(_Context *ctx) {
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
        ctx->width       = ctx->shm->width;
        ctx->height      = ctx->shm->height;

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
                    return 0;
                }
                ctx->pixels     = new_pixels;
                ctx->alloc_size = nbytes;
            }

            rewind(fp);
            fread(ctx->pixels, 1, nbytes, fp);
            fclose(fp);

            ctx->has_new_data = true;
        }
    }

    // Signal done reading
    memcpy(&ctx->shm->reading, &off, 4);

    return ctx->has_new_data ? 1 : 0;
}

#else

typedef int _dc_ps_shmem_c_unused; // !_WIN32

#endif // !_WIN32
