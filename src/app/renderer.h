#ifndef DC_APP_RENDERER_H
#define DC_APP_RENDERER_H

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppRenderer DcAppRenderer;

struct DcAppContext;
struct DcAppDrawContext;
struct DcAppFontContext;
struct DcAppLogicContext;
struct DcAppPixelstreamContext;
struct DcAppPlanetContext;
struct DcAppSceneContext;
struct DcAppTextureContext;

void dc_app_renderer_init(plApiRegistryI *api_registry);

DcAppRenderer *dc_app_renderer_create(
    struct DcAppContext *callback_context,
    struct DcAppSceneContext *scene,
    struct DcAppFontContext *fonts,
    struct DcAppTextureContext *textures,
    struct DcAppPlanetContext *planets,
    struct DcAppPixelstreamContext *pixelstreams,
    struct DcAppLogicContext *logic);
void dc_app_renderer_destroy(DcAppRenderer *renderer);

// Creates runtime planet resources after XML parsing has completed.
void dc_app_renderer_initialize_planets(DcAppRenderer *renderer);
// Applies variable-driven planet state before each render pass.
void dc_app_renderer_update_planets(DcAppRenderer *renderer);
void dc_app_renderer_render(DcAppRenderer *renderer, struct DcAppDrawContext *draw_ctx);
// Applies deferred Set operations after traversal so siblings see the original values.
void dc_app_renderer_flush_deferred_sets(DcAppRenderer *renderer);

#endif
