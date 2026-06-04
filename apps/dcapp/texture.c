#include "texture.h"

#include "utils/file.h"
#include "utils/log.h"
#include "utils/string.h"

static bool _texture_resolve_path(const char *path, const char *base_directory, char *out, size_t out_size);
static _TextureIndex _texture_find_by_name(_AppData *app_data, const char *canon_path);
static bool _texture_ensure_staging_buffer(_AppData *app_data, size_t required_size);
static bool _texture_upload_image(_AppData *app_data, _Texture texture, const unsigned char *image_data, uint32_t width, uint32_t height);
static bool _texture_get_index_size(_AppData *app_data, _TextureIndex texture_index, DcAppVec2 *out_size);
static bool _texture_upload_size_valid(int width, int height, size_t *out_size);

static const DcAppTextureApi dc_app_texture_interface = {
    .load_image = dc_app_texture_load_image,
    .get_size   = dc_app_texture_get_size,
};

const DcAppTextureApi *dc_app_texture_api(void) {
    return &dc_app_texture_interface;
}

_Texture dc_app_texture_create(_AppData *app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name, bool use_dedicated_allocator) {

    // get device
    plDevice *device = _ext_starter->get_device();

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
    plTextureHandle pl_texture_handle = _ext_gfx->create_texture(device, &pl_texture_desc, &pl_texture);

    // choose allocator based on texture size or caller request
    // use dedicated allocator for large textures (> 4 MB) to avoid buddy allocator waste
    plDeviceMemoryAllocatorI *allocator = app_data->gpu_local_buddy_allocator;
    if (use_dedicated_allocator || pl_texture->tMemoryRequirements.ulSize > (4 * 1048576))
        allocator = app_data->gpu_local_dedicated_allocator;

    const plDeviceMemoryAllocation pl_texture_allocation = allocator->allocate(
        allocator->ptInst,
        pl_texture->tMemoryRequirements.uMemoryTypeBits,
        pl_texture->tMemoryRequirements.ulSize,
        pl_texture->tMemoryRequirements.ulAlignment,
        texture_name);

    // bind memory
    _ext_gfx->bind_texture_to_memory(device, pl_texture_handle, &pl_texture_allocation);

    // create bind group
    plBindGroupHandle pl_bind_group_handle = _ext_dc_draw_backend->create_bind_group_for_texture(pl_texture_handle);

    // create _Texture struct
    _Texture texture = {
        pl_texture_handle,
        pl_bind_group_handle};
    return texture;
}

_TextureIndex dc_app_texture_load_image_index(_AppData *app_data, const char *path, const char *base_directory) {
    if (!app_data) {
        DC_LOG_ERROR("Image", "Failed to load image: missing app runtime");
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

    _TextureIndex texture_index = _texture_find_by_name(app_data, canon_path);
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

    size_t upload_size = 0;
    if (!_texture_upload_size_valid(image_width, image_height, &upload_size)) {
        DC_LOG_ERROR("Image", "Invalid image dimensions for file: %s", canon_path);
        _ext_image->free(image_data);
        return TEXTURE_INDEX_UNDEFINED;
    }

    if (!_texture_ensure_staging_buffer(app_data, upload_size)) {
        DC_LOG_ERROR("Image", "Failed to grow staging buffer for file: %s", canon_path);
        _ext_image->free(image_data);
        return TEXTURE_INDEX_UNDEFINED;
    }

    _Texture texture = dc_app_texture_create(app_data, (uint32_t)image_width, (uint32_t)image_height, canon_path, false);
    if (!_texture_upload_image(app_data, texture, image_data, (uint32_t)image_width, (uint32_t)image_height)) {
        DC_LOG_ERROR("Image", "Failed to upload file: %s", canon_path);
        _ext_image->free(image_data);
        return TEXTURE_INDEX_UNDEFINED;
    }

    _ext_image->free(image_data);

    sbpush(app_data->sb_texture_name_offsets, sbcount(app_data->sb_texture_names));
    sbpushn(app_data->sb_texture_names, canon_path, (int)strlen(canon_path) + 1);
    sbpush(app_data->sb_textures, texture);
    texture_index = sbcount(app_data->sb_textures) - 1;
    DC_LOG_INFO("Image", "Loaded image texture %d: %s (%dx%d)", texture_index, canon_path, image_width, image_height);
    return texture_index;
}

DcAppTextureId dc_app_texture_load_image(void *user_data, const char *path, DcAppVec2 *out_size) {
    _AppData *app_data = (_AppData *)user_data;
    if (out_size) *out_size = (DcAppVec2){0};

    _TextureIndex texture_index = dc_app_texture_load_image_index(app_data, path, app_data && app_data->config ? app_data->config->config_dir_path : NULL);
    if (texture_index == TEXTURE_INDEX_UNDEFINED) return 0;

    _texture_get_index_size(app_data, texture_index, out_size);
    return (DcAppTextureId)texture_index;
}

bool dc_app_texture_get_size(void *user_data, DcAppTextureId texture_id, DcAppVec2 *out_size) {
    _AppData *app_data = (_AppData *)user_data;
    if (out_size) *out_size = (DcAppVec2){0};
    if (!app_data || texture_id == 0) return false;
    return _texture_get_index_size(app_data, (_TextureIndex)texture_id, out_size);
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

static _TextureIndex _texture_find_by_name(_AppData *app_data, const char *canon_path) {
    if (!app_data || !canon_path) return TEXTURE_INDEX_UNDEFINED;

    for (int i = TEXTURE_FIRST_INDEX; i < sbcount(app_data->sb_textures); i++) {
        const char *texture_name = &(app_data->sb_texture_names[app_data->sb_texture_name_offsets[i]]);
        if (strcmp(canon_path, texture_name) == 0) {
            return i;
        }
    }
    return TEXTURE_INDEX_UNDEFINED;
}

static bool _texture_ensure_staging_buffer(_AppData *app_data, size_t required_size) {
    if (!app_data) return false;
    if (required_size <= app_data->pl_staging_buffer_size) return true;

    plDevice *device = _ext_starter->get_device();
    _ext_gfx->destroy_buffer(device, app_data->pl_staging_buffer_handle);

    const plBufferDesc staging_buffer_desc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = required_size,
        .pcDebugName = "staging buffer"};
    app_data->pl_staging_buffer_handle = _ext_gfx->create_buffer(device, &staging_buffer_desc, NULL);
    app_data->pl_staging_buffer_size   = required_size;

    plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
    plDeviceMemoryAllocatorI *allocator = app_data->gpu_staging_uncached_allocator;
    const plDeviceMemoryAllocation staging_buffer_allocation = allocator->allocate(
        allocator->ptInst,
        staging_buffer->tMemoryRequirements.uMemoryTypeBits,
        staging_buffer->tMemoryRequirements.ulSize,
        staging_buffer->tMemoryRequirements.ulAlignment,
        "staging buffer memory");
    _ext_gfx->bind_buffer_to_memory(device, app_data->pl_staging_buffer_handle, &staging_buffer_allocation);
    return true;
}

static bool _texture_upload_image(_AppData *app_data, _Texture texture, const unsigned char *image_data, uint32_t width, uint32_t height) {
    if (!app_data || !image_data || width == 0 || height == 0) return false;

    plDevice *device = _ext_starter->get_device();

    plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
    _ext_gfx->set_texture_usage(encoder, texture.texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);

    plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
    memcpy(staging_buffer->tMemoryAllocation.pHostMapped, image_data, (size_t)width * (size_t)height * 4);

    plBufferImageCopy buffer_image_copy;
    memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
    buffer_image_copy.uImageWidth    = width;
    buffer_image_copy.uImageHeight   = height;
    buffer_image_copy.uImageDepth    = 1;
    buffer_image_copy.uLayerCount    = 1;
    buffer_image_copy.szBufferOffset = 0;
    _ext_gfx->copy_buffer_to_texture(encoder, app_data->pl_staging_buffer_handle, texture.texture_handle, 1, &buffer_image_copy);

    _ext_starter->return_blit_encoder(encoder);
    return true;
}

static bool _texture_get_index_size(_AppData *app_data, _TextureIndex texture_index, DcAppVec2 *out_size) {
    if (!app_data || texture_index <= TEXTURE_INDEX_UNDEFINED || texture_index >= sbcount(app_data->sb_textures)) return false;

    plDevice  *device  = _ext_starter->get_device();
    plTexture *texture = _ext_gfx->get_texture(device, app_data->sb_textures[texture_index].texture_handle);
    if (!texture) return false;

    if (out_size) {
        out_size->x = texture->tDesc.tDimensions.x;
        out_size->y = texture->tDesc.tDimensions.y;
    }
    return true;
}

static bool _texture_upload_size_valid(int width, int height, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (width <= 0 || height <= 0) return false;
    if ((size_t)width > SIZE_MAX / (size_t)height) return false;

    size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX / 4) return false;

    if (out_size) *out_size = pixel_count * 4;
    return true;
}
