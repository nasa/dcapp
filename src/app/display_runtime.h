#ifndef DC_APP_DISPLAY_RUNTIME_H
#define DC_APP_DISPLAY_RUNTIME_H

//~ types

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppDisplayRuntimeContext DcAppDisplayRuntimeContext;

struct DcAppContext;
struct DcAppDrawContext;
struct DcAppFontContext;
struct DcAppDisplayLogicContext;
struct DcAppPixelstreamContext;
struct DcAppPlanetContext;
struct DcAppDisplayModelContext;
struct DcAppTextureContext;

//~ lifecycle

void dc_app_display_runtime_init(plApiRegistryI *api_registry);

DcAppDisplayRuntimeContext *dc_app_display_runtime_context_create(
    struct DcAppContext *callback_context,
    struct DcAppDisplayModelContext *model,
    struct DcAppFontContext *fonts,
    struct DcAppTextureContext *textures,
    struct DcAppPlanetContext *planets,
    struct DcAppPixelstreamContext *pixelstreams,
    struct DcAppDisplayLogicContext *display_logic);
void dc_app_display_runtime_context_destroy(DcAppDisplayRuntimeContext *runtime);

//~ frame processing

// creates runtime planet resources after xml parsing has completed
void dc_app_display_runtime_initialize_planets(DcAppDisplayRuntimeContext *runtime);
// applies variable-driven planet state before each render pass
void dc_app_display_runtime_update_planets(DcAppDisplayRuntimeContext *runtime);
void dc_app_display_runtime_render(DcAppDisplayRuntimeContext *runtime, struct DcAppDrawContext *draw_ctx);
// applies deferred set operations after traversal so siblings see the original values
void dc_app_display_runtime_flush_deferred_sets(DcAppDisplayRuntimeContext *runtime);

#endif
