#include "pl.h"
#include "pl_starter_ext.h"
#include "pl_gpu_allocators_ext.h"
#include "pl_graphics_ext.h"
#include "pl_image_ext.h"
#include "dc_draw_backend_ext.h"

#include "texture.h"

#include "app/vector.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/stb_sb.h"
#include "utils/string.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct _DcAppTexture {
    plTextureHandle   texture_handle;
    plBindGroupHandle bind_group_handle;
    uint32_t          width;
    uint32_t          height;
} _DcAppTexture;

struct DcAppTextureContext {
    plDevice *device;

    plDeviceMemoryAllocatorI *gpu_local_dedicated_allocator;
    plDeviceMemoryAllocatorI *gpu_local_buddy_allocator;
    plDeviceMemoryAllocatorI *gpu_staging_uncached_allocator;

    // One staging buffer grows to the largest upload seen by this context.
    plBufferHandle staging_buffer_handle;
    size_t         staging_buffer_size;
    bool           has_staging_buffer;

    char *asset_root;

    // Name offsets remain valid when the backing string buffer moves.
    char         *sb_texture_names;
    int          *sb_texture_name_offsets;
    _DcAppTexture *sb_textures;
};

static const plMemoryI        *_ext_memory          = NULL;
static const plStarterI       *_ext_starter         = NULL;
static const plGraphicsI      *_ext_gfx             = NULL;
static const plGPUAllocatorsI *_ext_gpu_allocators  = NULL;
static const dcDrawBackendI   *_ext_dc_draw_backend = NULL;
static const plImageI         *_ext_image           = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

static bool          _texture_resolve_path(const char *path, const char *base_directory, char *out, size_t out_size);
static DcAppTextureIndex _texture_find_by_name(DcAppTextureContext *texture_ctx, const char *canon_path);
static bool          _texture_ensure_staging_buffer(DcAppTextureContext *texture_ctx, size_t required_size);
static bool          _texture_upload_size_valid(uint32_t width, uint32_t height, size_t *out_size);
static bool          _texture_index_valid(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id);
static _DcAppTexture _texture_create_gpu(DcAppTextureContext *texture_ctx, uint32_t texture_width, uint32_t texture_height, const char *texture_name, bool use_dedicated_allocator);
static DcAppTextureIndex _texture_register(DcAppTextureContext *texture_ctx, _DcAppTexture texture, const char *name);

void dc_app_texture_init(plApiRegistryI *api_registry) {
    _ext_memory          = pl_get_api_latest(api_registry, plMemoryI);
    _ext_starter         = pl_get_api_latest(api_registry, plStarterI);
    _ext_gfx             = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_gpu_allocators  = pl_get_api_latest(api_registry, plGPUAllocatorsI);
    _ext_dc_draw_backend = pl_get_api_latest(api_registry, dcDrawBackendI);
    _ext_image           = pl_get_api_latest(api_registry, plImageI);
}

DcAppTextureContext *dc_app_texture_context_create(const char *asset_root) {
    DcAppTextureContext *texture_ctx = PL_ALLOC(sizeof(*texture_ctx));
    if (!texture_ctx) return NULL;
    memset(texture_ctx, 0, sizeof(*texture_ctx));

    // get device
    texture_ctx->device                          = _ext_starter->get_device();
    texture_ctx->gpu_local_dedicated_allocator  = _ext_gpu_allocators->get_local_dedicated_allocator(texture_ctx->device);
    texture_ctx->gpu_local_buddy_allocator      = _ext_gpu_allocators->get_local_buddy_allocator(texture_ctx->device);
    texture_ctx->gpu_staging_uncached_allocator = _ext_gpu_allocators->get_staging_uncached_allocator(texture_ctx->device);

    if (asset_root) {
        const size_t asset_root_size = strlen(asset_root) + 1;
        texture_ctx->asset_root      = PL_ALLOC(asset_root_size);
        memcpy(texture_ctx->asset_root, asset_root, asset_root_size);
    }

    // Keep ID zero reserved so every real texture has a nonzero stable ID.
    sbresize(texture_ctx->sb_textures, 1);
    sbresize(texture_ctx->sb_texture_name_offsets, 1);
    sbresize(texture_ctx->sb_texture_names, 1);
    texture_ctx->sb_textures[0]            = (_DcAppTexture){0};
    texture_ctx->sb_texture_name_offsets[0] = 0;
    texture_ctx->sb_texture_names[0]        = '\0';
    return texture_ctx;
}

void dc_app_texture_context_destroy(DcAppTextureContext *texture_ctx) {
    if (!texture_ctx) return;

    // cleanup textures
    for (int i = TEXTURE_FIRST_INDEX; i < sbcount(texture_ctx->sb_textures); i++) {
        _ext_gfx->destroy_bind_group(texture_ctx->device, texture_ctx->sb_textures[i].bind_group_handle);
        _ext_gfx->destroy_texture(texture_ctx->device, texture_ctx->sb_textures[i].texture_handle);
    }
    if (texture_ctx->has_staging_buffer) {
        _ext_gfx->destroy_buffer(texture_ctx->device, texture_ctx->staging_buffer_handle);
    }

    sbfree(texture_ctx->sb_textures);
    sbfree(texture_ctx->sb_texture_names);
    sbfree(texture_ctx->sb_texture_name_offsets);
    if (texture_ctx->asset_root) PL_FREE(texture_ctx->asset_root);
    PL_FREE(texture_ctx);
}

DcAppTextureIndex dc_app_texture_create(DcAppTextureContext *texture_ctx, uint32_t width, uint32_t height, const char *name, bool use_dedicated_allocator) {
    if (!texture_ctx || width == 0 || height == 0) return TEXTURE_INDEX_UNDEFINED;
    return _texture_register(texture_ctx, _texture_create_gpu(texture_ctx, width, height, name, use_dedicated_allocator), name);
}

DcAppTextureIndex dc_app_texture_create_rgba(DcAppTextureContext *texture_ctx, uint32_t width, uint32_t height, const char *name, const void *rgba, bool use_dedicated_allocator) {
    if (!rgba) return TEXTURE_INDEX_UNDEFINED;

    DcAppTextureIndex texture_index = dc_app_texture_create(texture_ctx, width, height, name, use_dedicated_allocator);
    if (texture_index == TEXTURE_INDEX_UNDEFINED) return TEXTURE_INDEX_UNDEFINED;
    if (!dc_app_texture_update_rgba(texture_ctx, (DcAppTextureId)texture_index, rgba, width, height)) return TEXTURE_INDEX_UNDEFINED;
    return texture_index;
}

DcAppTextureIndex dc_app_texture_load_image_index(DcAppTextureContext *texture_ctx, const char *path, const char *base_directory) {
    if (!texture_ctx) {
        DC_LOG_ERROR("Image", "Failed to load image: missing texture context");
        return TEXTURE_INDEX_UNDEFINED;
    }
    if (!path || path[0] == '\0') {
        DC_LOG_ERROR("Image", "Failed to load image: empty path");
        return TEXTURE_INDEX_UNDEFINED;
    }

    char canon_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
    if (!_texture_resolve_path(path, base_directory, canon_path, sizeof(canon_path))) {
        DC_LOG_ERROR("Image", "Failed to resolve image path '%s' from base '%s'", path, base_directory ? base_directory : "");
        return TEXTURE_INDEX_UNDEFINED;
    }

    DcAppTextureIndex texture_index = _texture_find_by_name(texture_ctx, canon_path);
    if (texture_index != TEXTURE_INDEX_UNDEFINED) {
        return texture_index;
    }

    size_t         file_data_size = 0;
    unsigned char *file_data      = dc_utils_load_binary_file(canon_path, &file_data_size);
    if (!file_data) {
        DC_LOG_ERROR("Image", "Failed to load file: %s", canon_path);
        return TEXTURE_INDEX_UNDEFINED;
    }

    int            image_width, image_height, channels;
    unsigned char *image_data = _ext_image->load(file_data, (int)file_data_size, &image_width, &image_height, &channels, 4);
    (void)channels;
    free(file_data);
    if (!image_data) {
        DC_LOG_ERROR("Image", "Failed to decode file: %s", canon_path);
        return TEXTURE_INDEX_UNDEFINED;
    }

    if (image_width <= 0 || image_height <= 0) {
        DC_LOG_ERROR("Image", "Invalid image dimensions for file: %s", canon_path);
        _ext_image->free(image_data);
        return TEXTURE_INDEX_UNDEFINED;
    }

    texture_index = dc_app_texture_create_rgba(texture_ctx, (uint32_t)image_width, (uint32_t)image_height, canon_path, image_data, false);
    _ext_image->free(image_data);
    if (texture_index == TEXTURE_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Image", "Failed to upload file: %s", canon_path);
        return TEXTURE_INDEX_UNDEFINED;
    }

    DC_LOG_INFO("Image", "Loaded image texture %d: %s (%dx%d)", texture_index, canon_path, image_width, image_height);
    return texture_index;
}

DcAppTextureId dc_app_texture_load_image(DcAppTextureContext *texture_ctx, const char *path, DcAppVec2 *out_size) {
    if (out_size) *out_size = (DcAppVec2){0};
    if (!texture_ctx) return 0;

    DcAppTextureIndex texture_index = dc_app_texture_load_image_index(texture_ctx, path, texture_ctx->asset_root);
    if (texture_index == TEXTURE_INDEX_UNDEFINED) return 0;

    dc_app_texture_get_size(texture_ctx, (DcAppTextureId)texture_index, out_size);
    return (DcAppTextureId)texture_index;
}

bool dc_app_texture_update_rgba(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, const void *rgba, uint32_t width, uint32_t height) {
    if (!_texture_index_valid(texture_ctx, texture_id) || !rgba) return false;

    _DcAppTexture *texture = &texture_ctx->sb_textures[texture_id];
    if (width == 0 || height == 0 || width > texture->width || height > texture->height) return false;

    size_t upload_size = 0;
    if (!_texture_upload_size_valid(width, height, &upload_size)) return false;
    if (!_texture_ensure_staging_buffer(texture_ctx, upload_size)) return false;

    plBuffer *staging_buffer = _ext_gfx->get_buffer(texture_ctx->device, texture_ctx->staging_buffer_handle);
    memcpy(staging_buffer->tMemoryAllocation.pHostMapped, rgba, upload_size);

    plBufferImageCopy buffer_image_copy;
    memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
    buffer_image_copy.uImageWidth    = width;
    buffer_image_copy.uImageHeight   = height;
    buffer_image_copy.uImageDepth    = 1;
    buffer_image_copy.uLayerCount    = 1;
    buffer_image_copy.szBufferOffset = 0;

    plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
    _ext_gfx->copy_buffer_to_texture(encoder, texture_ctx->staging_buffer_handle, texture->texture_handle, 1, &buffer_image_copy);
    _ext_starter->return_blit_encoder(encoder);
    return true;
}

bool dc_app_texture_get_size(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, DcAppVec2 *out_size) {
    if (out_size) *out_size = (DcAppVec2){0};
    if (!_texture_index_valid(texture_ctx, texture_id)) return false;

    if (out_size) {
        out_size->x = (float)texture_ctx->sb_textures[texture_id].width;
        out_size->y = (float)texture_ctx->sb_textures[texture_id].height;
    }
    return true;
}

bool dc_app_texture_get_bind_group(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, uint32_t *out_bind_group) {
    if (out_bind_group) *out_bind_group = 0;
    if (!_texture_index_valid(texture_ctx, texture_id) || !out_bind_group) return false;

    *out_bind_group = texture_ctx->sb_textures[texture_id].bind_group_handle.uData;
    return true;
}

bool dc_app_texture_get_bind_group_handle(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, plBindGroupHandle *out_bind_group) {
    if (out_bind_group) *out_bind_group = (plBindGroupHandle){0};
    if (!_texture_index_valid(texture_ctx, texture_id) || !out_bind_group) return false;

    *out_bind_group = texture_ctx->sb_textures[texture_id].bind_group_handle;
    return true;
}

bool dc_app_texture_get_texture_handle(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, plTextureHandle *out_texture) {
    if (out_texture) *out_texture = (plTextureHandle){0};
    if (!_texture_index_valid(texture_ctx, texture_id) || !out_texture) return false;

    *out_texture = texture_ctx->sb_textures[texture_id].texture_handle;
    return true;
}

static bool _texture_resolve_path(const char *path, const char *base_directory, char *out, size_t out_size) {
    if (!path || !out || out_size == 0) {
        return false;
    }

    char cleaned_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
    strncpy(cleaned_path, path, sizeof(cleaned_path) - 1);
    cleaned_path[sizeof(cleaned_path) - 1] = '\0';
    dc_utils_trim_whitespace_inplace(cleaned_path);
    if (cleaned_path[0] == '\0') {
        return false;
    }

    if (dc_utils_is_relative_path(cleaned_path)) {
        if (!base_directory || base_directory[0] == '\0') {
            return false;
        }

        char abs_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        if (dc_utils_join_paths(base_directory, cleaned_path, abs_path, sizeof(abs_path)) != 0) {
            return false;
        }
        if (dc_utils_canonicalize_path(abs_path, out, out_size) != 0) {
            DC_LOG_ERROR("Image", "Failed to canonicalize image path: %s", abs_path);
            return false;
        }
        return true;
    }

    if (dc_utils_canonicalize_path(cleaned_path, out, out_size) != 0) {
        DC_LOG_ERROR("Image", "Failed to canonicalize image path: %s", cleaned_path);
        return false;
    }
    return true;
}

static DcAppTextureIndex _texture_find_by_name(DcAppTextureContext *texture_ctx, const char *canon_path) {
    if (!texture_ctx || !canon_path) return TEXTURE_INDEX_UNDEFINED;

    for (int i = TEXTURE_FIRST_INDEX; i < sbcount(texture_ctx->sb_textures); i++) {
        const char *texture_name = &(texture_ctx->sb_texture_names[texture_ctx->sb_texture_name_offsets[i]]);
        if (strcmp(canon_path, texture_name) == 0) {
            return i;
        }
    }
    return TEXTURE_INDEX_UNDEFINED;
}

static bool _texture_ensure_staging_buffer(DcAppTextureContext *texture_ctx, size_t required_size) {
    if (required_size == 0) return false;
    if (texture_ctx->has_staging_buffer && required_size <= texture_ctx->staging_buffer_size) return true;

    if (texture_ctx->has_staging_buffer) {
        _ext_gfx->destroy_buffer(texture_ctx->device, texture_ctx->staging_buffer_handle);
    }

    const plBufferDesc staging_buffer_desc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = required_size,
        .pcDebugName = "staging buffer"};
    texture_ctx->staging_buffer_handle = _ext_gfx->create_buffer(texture_ctx->device, &staging_buffer_desc, NULL);
    texture_ctx->staging_buffer_size   = required_size;
    texture_ctx->has_staging_buffer    = true;

    plBuffer *staging_buffer = _ext_gfx->get_buffer(texture_ctx->device, texture_ctx->staging_buffer_handle);
    const plDeviceMemoryAllocation staging_buffer_allocation = texture_ctx->gpu_staging_uncached_allocator->allocate(
        texture_ctx->gpu_staging_uncached_allocator->ptInst,
        staging_buffer->tMemoryRequirements.uMemoryTypeBits,
        staging_buffer->tMemoryRequirements.ulSize,
        staging_buffer->tMemoryRequirements.ulAlignment,
        "staging buffer memory");
    _ext_gfx->bind_buffer_to_memory(texture_ctx->device, texture_ctx->staging_buffer_handle, &staging_buffer_allocation);
    return true;
}

static bool _texture_upload_size_valid(uint32_t width, uint32_t height, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (width == 0 || height == 0) return false;
    if ((size_t)width > SIZE_MAX / (size_t)height) return false;

    size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX / 4) return false;

    if (out_size) *out_size = pixel_count * 4;
    return true;
}

static bool _texture_index_valid(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id) {
    return texture_ctx && texture_id != 0 && texture_id < (DcAppTextureId)sbcount(texture_ctx->sb_textures);
}

static _DcAppTexture _texture_create_gpu(DcAppTextureContext *texture_ctx, uint32_t texture_width, uint32_t texture_height, const char *texture_name, bool use_dedicated_allocator) {
    // create new texture desc
    plTextureDesc pl_texture_desc;
    memset(&pl_texture_desc, 0, sizeof(plTextureDesc));
    pl_texture_desc.tDimensions = (plVec3){(float)texture_width, (float)texture_height, 1.0f};
    pl_texture_desc.tFormat     = PL_FORMAT_R8G8B8A8_UNORM;
    pl_texture_desc.uLayers     = 1;
    pl_texture_desc.uMips       = 1;
    pl_texture_desc.tType       = PL_TEXTURE_TYPE_2D;
    pl_texture_desc.tUsage      = PL_TEXTURE_USAGE_SAMPLED;
    pl_texture_desc.pcDebugName = texture_name;

    // create texture
    plTexture      *pl_texture;
    plTextureHandle pl_texture_handle = _ext_gfx->create_texture(texture_ctx->device, &pl_texture_desc, &pl_texture);

    // choose allocator based on texture size or caller request
    // use dedicated allocator for large textures (> 4 MB) to avoid buddy allocator waste
    plDeviceMemoryAllocatorI *allocator = texture_ctx->gpu_local_buddy_allocator;
    if (use_dedicated_allocator || pl_texture->tMemoryRequirements.ulSize > (4 * 1048576))
        allocator = texture_ctx->gpu_local_dedicated_allocator;

    const plDeviceMemoryAllocation pl_texture_allocation = allocator->allocate(
        allocator->ptInst,
        pl_texture->tMemoryRequirements.uMemoryTypeBits,
        pl_texture->tMemoryRequirements.ulSize,
        pl_texture->tMemoryRequirements.ulAlignment,
        texture_name);

    // bind memory
    _ext_gfx->bind_texture_to_memory(texture_ctx->device, pl_texture_handle, &pl_texture_allocation);

    // create bind group
    plBindGroupHandle pl_bind_group_handle = _ext_dc_draw_backend->create_bind_group_for_texture(pl_texture_handle);

    // Establish the sampled layout once. Later uploads preserve that layout.
    plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
    _ext_gfx->set_texture_usage(encoder, pl_texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);
    _ext_starter->return_blit_encoder(encoder);

    // create _Texture struct
    _DcAppTexture texture = {
        pl_texture_handle,
        pl_bind_group_handle,
        texture_width,
        texture_height};
    return texture;
}

static DcAppTextureIndex _texture_register(DcAppTextureContext *texture_ctx, _DcAppTexture texture, const char *name) {
    const char *stored_name = name ? name : "";
    sbpush(texture_ctx->sb_texture_name_offsets, sbcount(texture_ctx->sb_texture_names));
    sbpushn(texture_ctx->sb_texture_names, stored_name, (int)strlen(stored_name) + 1);
    sbpush(texture_ctx->sb_textures, texture);
    return sbcount(texture_ctx->sb_textures) - 1;
}
