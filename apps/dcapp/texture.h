#ifndef _DCAPP_TEXTURE_H_
#define _DCAPP_TEXTURE_H_

#include "dcapp.h"
#include "draw.h"

_Texture dc_app_texture_create(_AppData *app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name, bool use_dedicated_allocator);
_TextureIndex dc_app_texture_load_image_index(_AppData *app_data, const char *path, const char *base_directory);
DcAppTextureId dc_app_texture_load_image(void *user_data, const char *path, DcAppVec2 *out_size);
bool dc_app_texture_get_size(void *user_data, DcAppTextureId texture_id, DcAppVec2 *out_size);

const DcAppTextureApi *dc_app_texture_api(void);

#endif
