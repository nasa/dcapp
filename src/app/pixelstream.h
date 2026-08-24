#ifndef DC_APP_PIXELSTREAM_H
#define DC_APP_PIXELSTREAM_H

#include "app/pixelstream_types.h"
#include "app/texture_types.h"

#include <stdbool.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppPixelstreamContext DcAppPixelstreamContext;
typedef struct DcAppPixelstreamState DcAppPixelstreamState;
struct DcAppTextureContext;

enum {
    DC_APP_PIXELSTREAM_SOURCE_INDEX_UNDEFINED = -1,
    DC_APP_PIXELSTREAM_MAX_WIDTH = 3840,
    DC_APP_PIXELSTREAM_MAX_HEIGHT = 2160,
};

struct DcAppPixelstreamState {
    DcAppTextureId texture;
    bool connected;
    int width;
    int height;
};

void dc_app_pixelstream_init(plApiRegistryI *api_registry);

DcAppPixelstreamContext *dc_app_pixelstream_context_create(struct DcAppTextureContext *textures);
void dc_app_pixelstream_context_destroy(DcAppPixelstreamContext *pixelstreams);

DcAppPixelstreamSourceIndex dc_app_pixelstream_find(DcAppPixelstreamContext *pixelstreams, DcAppPixelstreamType type, const char *source_key);
DcAppPixelstreamSourceIndex dc_app_pixelstream_add(DcAppPixelstreamContext *pixelstreams, DcAppPixelstreamType type, const char *source_key, int timeout);
bool dc_app_pixelstream_get_state(DcAppPixelstreamContext *pixelstreams, DcAppPixelstreamSourceIndex index, DcAppPixelstreamState *state);
int dc_app_pixelstream_get_count(const DcAppPixelstreamContext *pixelstreams);

void dc_app_pixelstream_update(DcAppPixelstreamContext *pixelstreams);

#endif
