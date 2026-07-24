#ifndef DC_APP_TEXTURE_API_H
#define DC_APP_TEXTURE_API_H

#include "app/texture_types.h"
#include "app/vector.h"

#include <stdbool.h>

typedef struct DcAppContext DcAppContext;
typedef struct DcAppTextureApi DcAppTextureApi;

struct DcAppTextureApi {
    DcAppTextureId (*load_image)(DcAppContext *app_ctx, const char *path, DcAppVec2 *out_size);
    bool (*get_size)(DcAppContext *app_ctx, DcAppTextureId texture_id, DcAppVec2 *out_size);
};

#endif
