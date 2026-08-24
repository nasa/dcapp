#include "pixelstream.h"

#include "pl.h"
#include "pl_image_ext.h"

#include "texture.h"
#include "pixelstream/mjpeg.h"
#include "pixelstream/shmem.h"
#include "utils/log.h"
#include "utils/stb_sb.h"

#include <stdlib.h>
#include <string.h>

// unique pixelstream source (shared across nodes with the same key)
typedef struct _DcAppPixelstreamSource {
    DcAppPixelstreamType type;
    DcAppTextureId texture;
    bool is_connected;

    // frame data (fetched once per frame by the source, not per-node)
    unsigned char *frame;
    int frame_width;
    int frame_height;

    union {
        struct {
            DcPsMjpegServer *server;
            unsigned char *raw_jpeg;
            size_t raw_jpeg_size;
        } mjpeg;
        struct {
            DcPsShmemSource *source;
        } shmem;
    };
} _DcAppPixelstreamSource;

struct DcAppPixelstreamContext {
    DcAppTextureContext *textures;
    DcPsMjpegContext *mjpeg;
    DcPsShmemContext *shmem;

    // pixelstream sources (unique, deduplicated by source key during XML parse)
    _DcAppPixelstreamSource *sb_sources;
    char *sb_source_keys;       // stretchy buffer of null-terminated key strings
    int *sb_source_key_offsets; // offset into sb_source_keys for each source
};

static const plMemoryI *_ext_memory = NULL;
static const plImageI *_ext_image = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x) _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_pixelstream_init(plApiRegistryI *api_registry) {
    _ext_memory = pl_get_api_latest(api_registry, plMemoryI);
    _ext_image = pl_get_api_latest(api_registry, plImageI);
}

DcAppPixelstreamContext *dc_app_pixelstream_context_create(DcAppTextureContext *textures) {
    DcAppPixelstreamContext *pixelstreams = PL_ALLOC(sizeof(*pixelstreams));
    if (!pixelstreams) return NULL;

    memset(pixelstreams, 0, sizeof(*pixelstreams));
    pixelstreams->textures = textures;

    pixelstreams->mjpeg = dc_ps_mjpeg_context_create();
    pixelstreams->shmem = dc_ps_shmem_context_create();
    return pixelstreams;
}

void dc_app_pixelstream_context_destroy(DcAppPixelstreamContext *pixelstreams) {
    if (!pixelstreams) return;

    // cleanup pixelstream sources
    for (int i = 0; i < sbcount(pixelstreams->sb_sources); i++) {
        _DcAppPixelstreamSource *src = &pixelstreams->sb_sources[i];
        if (src->type == DC_APP_PIXELSTREAM_TYPE_MJPEG) {
            if (src->frame) {
                _ext_image->free(src->frame);
            }
            if (src->mjpeg.raw_jpeg) {
                free(src->mjpeg.raw_jpeg);
            }
        } else if (src->type == DC_APP_PIXELSTREAM_TYPE_SHMEM) {
            if (src->frame) {
                free(src->frame);
            }
        }
    }

    sbfree(pixelstreams->sb_sources);
    sbfree(pixelstreams->sb_source_keys);
    sbfree(pixelstreams->sb_source_key_offsets);

    // cleanup pixelstream global contexts
    dc_ps_mjpeg_context_destroy(pixelstreams->mjpeg);
    dc_ps_shmem_context_destroy(pixelstreams->shmem);
    PL_FREE(pixelstreams);
}

DcAppPixelstreamSourceIndex dc_app_pixelstream_find(DcAppPixelstreamContext *pixelstreams, DcAppPixelstreamType type, const char *source_key) {
    if (!pixelstreams) return DC_APP_PIXELSTREAM_SOURCE_INDEX_UNDEFINED;

    const char *key = source_key ? source_key : "";

    // check existing sources
    for (int ii = 0; ii < sbcount(pixelstreams->sb_sources); ii++) {
        const char *existing_key = &pixelstreams->sb_source_keys[pixelstreams->sb_source_key_offsets[ii]];
        if (pixelstreams->sb_sources[ii].type == type && strcmp(existing_key, key) == 0) {
            return ii;
        }
    }
    return DC_APP_PIXELSTREAM_SOURCE_INDEX_UNDEFINED;
}

DcAppPixelstreamSourceIndex dc_app_pixelstream_add(DcAppPixelstreamContext *pixelstreams, DcAppPixelstreamType type, const char *source_key, int timeout) {
    if (!pixelstreams) return DC_APP_PIXELSTREAM_SOURCE_INDEX_UNDEFINED;

    const char *key = source_key ? source_key : "";

    // create new source if not found
    _DcAppPixelstreamSource src = {0};
    src.type = type;
    src.frame = NULL;
    src.frame_width = 0;
    src.frame_height = 0;
    src.is_connected = false;

    // create GPU texture
    src.texture = (DcAppTextureId)dc_app_texture_create(
        pixelstreams->textures,
        DC_APP_PIXELSTREAM_MAX_WIDTH,
        DC_APP_PIXELSTREAM_MAX_HEIGHT,
        "pixelstream",
        true);

    // create source
    switch (type) {
        case DC_APP_PIXELSTREAM_TYPE_SHMEM:
            src.shmem.source = dc_ps_shmem_add_source(pixelstreams->shmem, key);
            break;
        case DC_APP_PIXELSTREAM_TYPE_MJPEG:
            src.mjpeg.server = dc_ps_mjpeg_add_server(pixelstreams->mjpeg, key, timeout);
            src.mjpeg.raw_jpeg_size = DC_APP_PIXELSTREAM_MAX_WIDTH * DC_APP_PIXELSTREAM_MAX_HEIGHT * 4;
            src.mjpeg.raw_jpeg = (unsigned char *)malloc(src.mjpeg.raw_jpeg_size);
            break;
        default:
            DC_LOG_ERROR("PixelStream", "Unknown pixelstream type");
            break;
    }

    // store key
    sbpush(pixelstreams->sb_source_key_offsets, sbcount(pixelstreams->sb_source_keys));
    sbpushn(pixelstreams->sb_source_keys, key, (int)strlen(key) + 1);

    // store source
    sbpush(pixelstreams->sb_sources, src);
    return sbcount(pixelstreams->sb_sources) - 1;
}

bool dc_app_pixelstream_get_state(DcAppPixelstreamContext *pixelstreams, DcAppPixelstreamSourceIndex index, DcAppPixelstreamState *state) {
    if (!pixelstreams || !state || index < 0 || index >= sbcount(pixelstreams->sb_sources)) return false;
    const _DcAppPixelstreamSource *source = &pixelstreams->sb_sources[index];
    *state = (DcAppPixelstreamState){
        .texture = source->texture,
        .connected = source->is_connected,
        .width = source->frame_width,
        .height = source->frame_height,
    };
    return true;
}

int dc_app_pixelstream_get_count(const DcAppPixelstreamContext *pixelstreams) {
    return pixelstreams ? sbcount(pixelstreams->sb_sources) : 0;
}

void dc_app_pixelstream_update(DcAppPixelstreamContext *pixelstreams) {
    if (!pixelstreams) return;

    dc_ps_mjpeg_update(pixelstreams->mjpeg);
    dc_ps_shmem_update(pixelstreams->shmem);

    for (int ii = 0; ii < sbcount(pixelstreams->sb_sources); ii++) {
        _DcAppPixelstreamSource *src = &pixelstreams->sb_sources[ii];

        switch (src->type) {
            case DC_APP_PIXELSTREAM_TYPE_MJPEG: {
                src->is_connected = dc_ps_mjpeg_server_is_connected(src->mjpeg.server);
                if (!src->is_connected) break;

                if (dc_ps_mjpeg_server_has_new_data(src->mjpeg.server)) {
                    size_t jpeg_size;
                    dc_ps_mjpeg_get_server_data(src->mjpeg.server, src->mjpeg.raw_jpeg, src->mjpeg.raw_jpeg_size, &jpeg_size);

                    _ext_image->free(src->frame);

                    int channels;
                    src->frame = _ext_image->load(src->mjpeg.raw_jpeg, (int)jpeg_size, &src->frame_width, &src->frame_height, &channels, 4);

                    if (src->frame_height * src->frame_width > DC_APP_PIXELSTREAM_MAX_WIDTH * DC_APP_PIXELSTREAM_MAX_HEIGHT) {
                        DC_LOG_WARN("PixelStream", "Max image dimensions exceeded");
                        _ext_image->free(src->frame);
                        src->frame = NULL;
                        break;
                    }
                }
                break;
            }

            case DC_APP_PIXELSTREAM_TYPE_SHMEM: {
                src->is_connected = dc_ps_shmem_is_connected(src->shmem.source);
                if (!src->is_connected) break;

                if (dc_ps_shmem_has_new_data(src->shmem.source)) {
                    uint32_t width = dc_ps_shmem_get_width(src->shmem.source);
                    uint32_t height = dc_ps_shmem_get_height(src->shmem.source);

                    if (width * height > DC_APP_PIXELSTREAM_MAX_WIDTH * DC_APP_PIXELSTREAM_MAX_HEIGHT) {
                        DC_LOG_WARN("PixelStream", "Shmem max image dimensions exceeded");
                        break;
                    }

                    size_t frame_size = (size_t)width * height * 4;
                    void *tmp = realloc(src->frame, frame_size);
                    if (!tmp) {
                        DC_LOG_ERROR("PixelStream", "realloc failed for frame buffer");
                        break;
                    }
                    src->frame = tmp;

                    size_t out_size;
                    dc_ps_shmem_get_data(src->shmem.source, src->frame, frame_size, &out_size);

                    src->frame_width = (int)width;
                    src->frame_height = (int)height;
                }
                break;
            }

            default:
                break;
        }

        // upload to GPU if we have frame data
        if (src->frame && src->is_connected) {
            dc_app_texture_update_rgba(
                pixelstreams->textures,
                src->texture,
                src->frame,
                (uint32_t)src->frame_width,
                (uint32_t)src->frame_height);
        }
    }
}
