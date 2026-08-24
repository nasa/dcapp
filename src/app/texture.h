#ifndef DC_APP_TEXTURE_H
#define DC_APP_TEXTURE_H

#include "app/texture_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef union plBindGroupHandle plBindGroupHandle;
typedef union plTextureHandle plTextureHandle;
typedef struct DcAppTextureContext DcAppTextureContext;
union DcAppVec2;

enum {
    TEXTURE_INDEX_UNDEFINED = 0,
    TEXTURE_FIRST_INDEX = 1,
};

void dc_app_texture_init(plApiRegistryI *api_registry);

DcAppTextureContext *dc_app_texture_context_create(const char *asset_root);
void dc_app_texture_context_destroy(DcAppTextureContext *texture_ctx);

DcAppTextureIndex dc_app_texture_create(DcAppTextureContext *texture_ctx, uint32_t width, uint32_t height, const char *name, bool use_dedicated_allocator);
DcAppTextureIndex dc_app_texture_create_rgba(DcAppTextureContext *texture_ctx, uint32_t width, uint32_t height, const char *name, const void *rgba, bool use_dedicated_allocator);
DcAppTextureIndex dc_app_texture_load_image_index(DcAppTextureContext *texture_ctx, const char *path, const char *base_directory);
DcAppTextureId dc_app_texture_load_image(DcAppTextureContext *texture_ctx, const char *path, union DcAppVec2 *out_size);

bool dc_app_texture_update_rgba(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, const void *rgba, uint32_t width, uint32_t height);
bool dc_app_texture_get_size(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, union DcAppVec2 *out_size);
bool dc_app_texture_get_bind_group(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, uint32_t *out_bind_group);
bool dc_app_texture_get_bind_group_handle(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, plBindGroupHandle *out_bind_group);
bool dc_app_texture_get_texture_handle(DcAppTextureContext *texture_ctx, DcAppTextureId texture_id, plTextureHandle *out_texture);

#endif
