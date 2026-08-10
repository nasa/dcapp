#define _USE_MATH_DEFINES
#define PL_MATH_INCLUDE_FUNCTIONS
#include <math.h>
#include <string.h>

#include "draw.h"
#include "draw_internal.h"
#include "planet.h"
#include "texture.h"

#include "dc_draw_backend_ext.h"
#include "geo.h"
#include "geojson.h"
#include "pl.h"
#include "pl_camera_ext.h"
#include "pl_graphics_ext.h"
#include "pl_planet_ext.h"
#include "pl_starter_ext.h"
#include "utils/log.h"
#include "utils/math.h"
#include "utils/stb_sb.h"

typedef enum _DcAppStencilPhase {
    _DC_APP_STENCIL_PHASE_NONE,
    _DC_APP_STENCIL_PHASE_CREATE,
    _DC_APP_STENCIL_PHASE_REMOVE,
    _DC_APP_STENCIL_PHASE_DRAW,
    _DC_APP_STENCIL_PHASE_CLEANUP,
} _DcAppStencilPhase;

typedef struct _DcAppStencilFrame {
    _DcAppStencilPhase previous_phase;
} _DcAppStencilFrame;

typedef struct _DcAppStencilRecorder {
    _DcAppStencilFrame *sb_frames;
    _DcAppStencilPhase phase;
} _DcAppStencilRecorder;

// Split batches when drawing switches between 2D and 3D so submission preserves call order.
typedef enum __DrawBatchType {
    DRAW_BATCH_TYPE_UNDEFINED,
    DRAW_BATCH_TYPE_2D,
    DRAW_BATCH_TYPE_3D,
} _DrawBatchType;

typedef struct __DrawList2D {
    dcDrawList2D  *draw_list;
    dcDrawLayer2D *layer;
} _DrawList2D;

typedef struct __DrawBatch {
    _DrawBatchType type;
    union {
        _DrawList2D   draw_list_2d;
        dcDrawList3D *draw_list_3d;
    };
} _DrawBatch;

typedef struct _DcAppPlanetContainerFrame {
    DcAppDrawPlanetViewHandle draw_view;
    plVec3d up;
    plVec3d east;
    plVec3d north;
    double planet_radius;
    double surface_radius;
    double scale;
    double rotation_cos;
    double rotation_sin;
} _DcAppPlanetContainerFrame;

typedef enum _DcAppDrawMouseTargetType {
    _DC_APP_DRAW_MOUSE_TARGET_UNDEFINED,
    _DC_APP_DRAW_MOUSE_TARGET_INTERNAL,
    _DC_APP_DRAW_MOUSE_TARGET_ID,
} _DcAppDrawMouseTargetType;

typedef struct _DcAppDrawMouseTarget {
    _DcAppDrawMouseTargetType type;
    union {
        DcAppDrawTargetId target;
        uint64_t id;
    };
} _DcAppDrawMouseTarget;

struct DcAppDrawContext {
    dcFont *default_font;
    DcAppTextureContext *texture_ctx;

    DcAppDrawArea area;
    DcAppMouse screen_mouse;
    DcAppMouse mouse;

    _DcAppDrawMouseTarget pressed_target;
    _DcAppDrawMouseTarget next_pressed_target;
    _DcAppDrawMouseTarget hovered_target;
    _DcAppDrawMouseTarget next_hovered_target;
    _DcAppDrawMouseTarget released_target;
    _DcAppDrawMouseTarget active_target;

    DcAppDrawArea *sb_container_stack;
    DcAppDrawScope *sb_scope_stack;

    _DcAppStencilRecorder stencil;

    // queues planet views so they render to textures before entering the 2d draw stream.
    DcAppDrawPlanetViewHandle *sb_planet_views;
    _DcAppPlanetContainerFrame *sb_planet_container_stack;

    _DrawBatch    *sb_draw_batches;
    _DrawList2D   *sb_draw_list_2d_pool;
    dcDrawList3D **sb_draw_list_3d_pool;
    int draw_list_2d_index;
    int draw_list_3d_index;
};

static const plMemoryI        *_ext_memory          = NULL;
static const plCameraI        *_ext_camera          = NULL;
static const dcDrawI          *_ext_dc_draw          = NULL;
static const dcDrawBackendI   *_ext_dc_draw_backend  = NULL;
static const plGraphicsI      *_ext_gfx             = NULL;
static const plIOI            *_ext_ioi             = NULL;
static const plPlanetI        *_ext_planet          = NULL;
static const plStarterI       *_ext_starter         = NULL;

#define PL_ALLOC(x)      _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) _ext_memory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x)       _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_draw_init(plApiRegistryI *api_registry) {
    _ext_memory          = pl_get_api_latest(api_registry, plMemoryI);
    _ext_camera          = pl_get_api_latest(api_registry, plCameraI);
    _ext_dc_draw          = pl_get_api_latest(api_registry, dcDrawI);
    _ext_dc_draw_backend  = pl_get_api_latest(api_registry, dcDrawBackendI);
    _ext_gfx             = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_ioi             = pl_get_api_latest(api_registry, plIOI);
    _ext_planet          = pl_get_api_latest(api_registry, plPlanetI);
    _ext_starter         = pl_get_api_latest(api_registry, plStarterI);
}

static const DcAppDrawApi dc_app_draw_interface = {
    .get_area                       = dc_app_draw_get_area,
    .line                           = dc_app_draw_line,
    .polyline                       = dc_app_draw_polyline,
    .polygon                        = dc_app_draw_polygon,
    .convex_polygon_filled          = dc_app_draw_convex_polygon_filled,
    .rounded_polygon                = dc_app_draw_rounded_polygon,
    .rounded_convex_polygon_filled  = dc_app_draw_rounded_convex_polygon_filled,
    .quad                           = dc_app_draw_quad,
    .quad_filled                    = dc_app_draw_quad_filled,
    .rounded_quad                   = dc_app_draw_rounded_quad,
    .rounded_quad_filled            = dc_app_draw_rounded_quad_filled,
    .image                          = dc_app_draw_image,
    .rect                           = dc_app_draw_rect,
    .rect_filled                    = dc_app_draw_rect_filled,
    .rounded_rect                   = dc_app_draw_rounded_rect,
    .rounded_rect_filled            = dc_app_draw_rounded_rect_filled,
    .circle                         = dc_app_draw_circle,
    .circle_filled                  = dc_app_draw_circle_filled,
    .ellipse                        = dc_app_draw_ellipse,
    .ellipse_filled                 = dc_app_draw_ellipse_filled,
    .text_size                      = dc_app_draw_text_size,
    .text                           = dc_app_draw_text,
    .line_ex                        = dc_app_draw_line_ex,
    .polyline_ex                    = dc_app_draw_polyline_ex,
    .polygon_ex                     = dc_app_draw_polygon_ex,
    .convex_polygon_filled_ex       = dc_app_draw_convex_polygon_filled_ex,
    .rounded_polygon_ex             = dc_app_draw_rounded_polygon_ex,
    .rounded_convex_polygon_filled_ex = dc_app_draw_rounded_convex_polygon_filled_ex,
    .quad_ex                        = dc_app_draw_quad_ex,
    .quad_filled_ex                 = dc_app_draw_quad_filled_ex,
    .rounded_quad_ex                = dc_app_draw_rounded_quad_ex,
    .rounded_quad_filled_ex         = dc_app_draw_rounded_quad_filled_ex,
    .image_ex                       = dc_app_draw_image_ex,
    .rect_ex                        = dc_app_draw_rect_ex,
    .rect_filled_ex                 = dc_app_draw_rect_filled_ex,
    .rounded_rect_ex                = dc_app_draw_rounded_rect_ex,
    .rounded_rect_filled_ex         = dc_app_draw_rounded_rect_filled_ex,
    .circle_ex                      = dc_app_draw_circle_ex,
    .circle_filled_ex               = dc_app_draw_circle_filled_ex,
    .ellipse_ex                     = dc_app_draw_ellipse_ex,
    .ellipse_filled_ex              = dc_app_draw_ellipse_filled_ex,
    .text_ex                        = dc_app_draw_text_ex,
    .container_push                 = dc_app_draw_container_push,
    .container_push_ex              = dc_app_draw_container_push_ex,
    .container_push_area            = dc_app_draw_container_push_area,
    .container_pop                  = dc_app_draw_container_pop,
    .stencil_begin                  = dc_app_draw_stencil_begin,
    .stencil_add                    = dc_app_draw_stencil_add,
    .stencil_remove                 = dc_app_draw_stencil_remove,
    .stencil_draw                   = dc_app_draw_stencil_draw,
    .stencil_end                    = dc_app_draw_stencil_end,
    .planet_view_geodetic           = dc_app_draw_planet_view_geodetic,
    .planet_view_cartesian          = dc_app_draw_planet_view_cartesian,
    .planet_container_push_geodetic = dc_app_draw_planet_container_push_geodetic,
    .planet_container_pop           = dc_app_draw_planet_container_pop,
    .planet_line_local              = dc_app_draw_planet_line_local,
    .planet_polygon_local           = dc_app_draw_planet_polygon_local,
    .planet_convex_polygon_filled_local = dc_app_draw_planet_convex_polygon_filled_local,
    .planet_sphere_geodetic         = dc_app_draw_planet_sphere_geodetic,
    .planet_sphere_cartesian        = dc_app_draw_planet_sphere_cartesian,
    .planet_line_geodetic           = dc_app_draw_planet_line_geodetic,
    .planet_line_cartesian          = dc_app_draw_planet_line_cartesian,
    .planet_polygon_geodetic        = dc_app_draw_planet_polygon_geodetic,
    .planet_polygon_cartesian       = dc_app_draw_planet_polygon_cartesian,
    .planet_convex_polygon_filled_geodetic = dc_app_draw_planet_convex_polygon_filled_geodetic,
    .planet_convex_polygon_filled_cartesian = dc_app_draw_planet_convex_polygon_filled_cartesian,
    .planet_ellipse_geodetic        = dc_app_draw_planet_ellipse_geodetic,
    .planet_ellipse_cartesian       = dc_app_draw_planet_ellipse_cartesian,
    .planet_ellipse_filled_geodetic = dc_app_draw_planet_ellipse_filled_geodetic,
    .planet_ellipse_filled_cartesian = dc_app_draw_planet_ellipse_filled_cartesian,
    .planet_image_geodetic          = dc_app_draw_planet_image_geodetic,
    .planet_image_cartesian         = dc_app_draw_planet_image_cartesian,
    .planet_text_geodetic           = dc_app_draw_planet_text_geodetic,
    .planet_text_cartesian          = dc_app_draw_planet_text_cartesian,
    .planet_geojson                 = dc_app_draw_planet_geojson,
};

static const DcAppMouseApi dc_app_mouse_interface = {
    .rect       = dc_app_mouse_rect,
    .circle     = dc_app_mouse_circle,
    .ellipse    = dc_app_mouse_ellipse,
    .polygon    = dc_app_mouse_polygon,
    .rect_ex    = dc_app_mouse_rect_ex,
    .circle_ex  = dc_app_mouse_circle_ex,
    .ellipse_ex = dc_app_mouse_ellipse_ex,
    .polygon_ex = dc_app_mouse_polygon_ex,
    .hovered    = dc_app_mouse_hovered,
    .pressed    = dc_app_mouse_pressed,
    .released   = dc_app_mouse_released,
    .active     = dc_app_mouse_active,
    .clicked    = dc_app_mouse_clicked,
    .down       = dc_app_mouse_down,
    .get_state  = dc_app_mouse_get_state,
};

const DcAppDrawApi *dc_app_draw_api(void) {
    return &dc_app_draw_interface;
}

const DcAppMouseApi *dc_app_mouse_api(void) {
    return &dc_app_mouse_interface;
}

struct DcAppDrawPlanetView {
    DcAppPlanetViewHandle view;
    plCamera camera;
    DcAppPlanetViewOptions options;
    DcAppDrawArea area;
    plVec2 logical_dimensions;
};

typedef struct _DcAppResolvedGeojsonStyle {
    double height_above_terrain;
    float line_width;
    DcAppVec4 line_color;
    DcAppVec4 fill_color;
    bool line_enabled;
    bool fill_enabled;
} _DcAppResolvedGeojsonStyle;

static dcDrawLayer2D *_draw_batch_get_2d(DcAppDrawContext *ctx);
static dcDrawList3D  *_draw_batch_get_3d(DcAppDrawContext *ctx);
static void _set_stencil_phase(DcAppDrawContext *ctx, _DcAppStencilPhase phase);
static void _restore_stencil_phase(DcAppDrawContext *ctx, _DcAppStencilPhase phase);
static dcDrawStencilState _stencil_state(_DcAppStencilPhase phase, int depth);
static dcDrawCommandState _command_state(DcAppDrawContext *ctx);
static bool _placement_is_default(DcAppPlacement placement);
static DcAppDrawArea *_draw_result_area(DcAppDrawResult *result);
static void _draw_context_update_mouse(DcAppDrawContext *ctx);
static void _draw_area_from_rect_points(float width, float height, plVec2 p0, plVec2 p1, plVec2 p3, DcAppDrawArea *out_area);
static void _resolve_rect_points(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 out[4], DcAppDrawArea *out_area);
static plVec2 *_alloc_resolved_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area);
static uint64_t _mouse_id(const char *id);
static bool _mouse_rect_local(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 *out);
static bool _mouse_point_in_polygon(plVec2 mouse, const plVec2 *points, uint32_t point_count);
static void _mouse_register(DcAppDrawContext *ctx, uint64_t id);
static bool _resolve_texture_id(DcAppDrawContext *ctx, DcAppTextureId texture_id, uint32_t *out);
static void _draw_image_uv(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 dimensions, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area);
static dcDrawTextOptions _text_options(DcAppDrawContext *ctx, DcAppTextStyle style);
static plMat3 _text_transform(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement);
static void _clear_stencil_bit(DcAppDrawContext *ctx);
static void _apply_planet_view_options(DcAppDrawPlanetViewHandle draw_view);
static void _flush_planet_views(DcAppDrawContext *ctx, int first_view);
static _DcAppPlanetContainerFrame *_planet_container_frame(DcAppDrawContext *ctx);
static plVec3 *_planet_container_transform_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count);
static plCamera _planet_camera_base(float fov_degrees, bool orthographic, DcAppVec2 size);
static plCamera _planet_camera_geodetic(DcAppPlanetHandle planet, double lat, double lon, double elevation, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppVec2 size);
static plCamera _planet_camera_cartesian(DcAppPlanetHandle planet, DcAppVec3d position, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppVec2 size);
static void     _planet_camera_apply_distance_ortho(DcAppPlanetHandle planet, plCamera *camera);
static bool     _planet_project_overlay(DcAppDrawPlanetViewHandle draw_view, DcAppVec3d position, float size_meters, DcAppVec2 *out_position, float *out_size);
static DcAppVec2 _planet_image_size_meters(DcAppDrawContext *ctx, DcAppTextureId texture_id, DcAppVec2 size);
static void     _planet_draw_image_label(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint);
static void     _planet_draw_text_label(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec2 position, const char *text, float size, DcAppVec4 color);
static _DcAppResolvedGeojsonStyle _planet_geojson_style(const DcGeojsonFeature *feature, DcAppPlanetGeojsonStyle fallback);
static plVec3  *_planet_geojson_points(DcAppPlanetHandle planet, const DcGeojsonCoordArray *coordinates, double height_above_terrain);
static void     _planet_draw_geojson_feature(DcAppDrawPlanetViewHandle draw_view, const DcGeojsonFeature *feature, _DcAppResolvedGeojsonStyle style);

//-----------------------------------------------------------------------------
// [SECTION] DrawFunction context helpers
//-----------------------------------------------------------------------------

DcAppDrawContext *dc_app_draw_context_create(dcFont *default_font, DcAppTextureContext *texture_ctx) {
    if (!texture_ctx) return NULL;

    DcAppDrawContext *ctx = PL_ALLOC(sizeof(*ctx));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(*ctx));
    ctx->default_font = default_font;
    ctx->texture_ctx  = texture_ctx;
    dc_app_draw_context_begin(ctx, (DcAppDrawFrameInput){0});
    return ctx;
}

void dc_app_draw_context_destroy(DcAppDrawContext *ctx) {
    if (!ctx) return;

    // cleans up every queued planet view.
    for (int i = 0; i < sbcount(ctx->sb_planet_views); i++) {
        PL_FREE(ctx->sb_planet_views[i]);
    }

    // cleanup draw batch system
    for (int i = 0; i < sbcount(ctx->sb_draw_list_2d_pool); i++) {
        _ext_dc_draw->return_2d_drawlist(ctx->sb_draw_list_2d_pool[i].draw_list);
    }
    for (int i = 0; i < sbcount(ctx->sb_draw_list_3d_pool); i++) {
        _ext_dc_draw->return_3d_drawlist(ctx->sb_draw_list_3d_pool[i]);
    }

    sbfree(ctx->sb_draw_batches);
    sbfree(ctx->sb_draw_list_2d_pool);
    sbfree(ctx->sb_draw_list_3d_pool);
    sbfree(ctx->stencil.sb_frames);
    sbfree(ctx->sb_planet_views);
    sbfree(ctx->sb_planet_container_stack);
    sbfree(ctx->sb_scope_stack);
    sbfree(ctx->sb_container_stack);
    PL_FREE(ctx);
}

void dc_app_draw_context_begin(DcAppDrawContext *ctx, DcAppDrawFrameInput input) {
    if (!ctx) return;

    bool was_mouse_down              = ctx->screen_mouse.down;
    ctx->screen_mouse.x              = input.mouse_position.x;
    ctx->screen_mouse.y              = input.mouse_position.y;
    ctx->screen_mouse.position_valid = input.mouse_position_valid;
    ctx->screen_mouse.pressed        = input.mouse_down && !was_mouse_down;
    ctx->screen_mouse.released       = !input.mouse_down && was_mouse_down;
    ctx->screen_mouse.down           = input.mouse_down;

    for (int ii = 0; ii < sbcount(ctx->sb_planet_views); ii++) {
        PL_FREE(ctx->sb_planet_views[ii]);
    }

    // clear the batches array (doesn't free memory, just resets count)
    sbclear(ctx->sb_draw_batches);
    sbclear(ctx->stencil.sb_frames);
    sbclear(ctx->sb_planet_views);
    sbclear(ctx->sb_planet_container_stack);
    sbclear(ctx->sb_scope_stack);
    sbclear(ctx->sb_container_stack);

    // reset pool indices
    ctx->draw_list_2d_index = 0;
    ctx->draw_list_3d_index = 0;
    ctx->stencil.phase      = _DC_APP_STENCIL_PHASE_NONE;

    ctx->area = (DcAppDrawArea){0};
    const plIO *io = _ext_ioi->get_io();
    ctx->area.dimensions[0] = io->tMainViewportSize.x;
    ctx->area.dimensions[1] = io->tMainViewportSize.y;
    plMat4 identity = pl_identity_mat4();
    memcpy(ctx->area.transform, identity.d, sizeof(ctx->area.transform));
    _draw_context_update_mouse(ctx);
}

void dc_app_draw_context_end(DcAppDrawContext *ctx) {
    if (!ctx) return;

    while (sbcount(ctx->sb_scope_stack) > 0) {
        DcAppDrawScope scope = ctx->sb_scope_stack[sbcount(ctx->sb_scope_stack) - 1];
        dc_app_draw_scope_end(ctx, scope);
    }

    DcAppDrawScope root = {0};
    plMat4 identity = pl_identity_mat4();
    memcpy(root.area.transform, identity.d, sizeof(root.area.transform));
    dc_app_draw_scope_end(ctx, root);
}

void dc_app_draw_context_submit(DcAppDrawContext *ctx, plRenderEncoder *encoder) {
    if (!ctx || !encoder) return;

    // submit draw lists from batch system in order
    plIO *ptIO = _ext_ioi->get_io();
    {
        const plSwapchainInfo swapchain_info = _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain());
        const dcDrawSubmitInfo draw_submit = {
            .tLogicalDimensions = ptIO->tMainViewportSize,
            .uFramebufferWidth = swapchain_info.uWidth,
            .uFramebufferHeight = swapchain_info.uHeight,
            .uMSAASampleCount = swapchain_info.tSampleCount,
        };

        // orthographic MVP for 3D objects in 2D space
        // Note: dcapp uses bottom-left origin, so Y is NOT flipped here (parent_transform handles it)
        float  w          = draw_submit.tLogicalDimensions.x;
        float  h          = draw_submit.tLogicalDimensions.y;
        float  n          = -1000.0f;
        float  f          = 1000.0f;
        plMat4 ortho_proj = {
            .col = {
                {2.0f / w, 0.0f, 0.0f, 0.0f},
                {0.0f, 2.0f / h, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f / (f - n), 0.0f},
                {-1.0f, -1.0f, -n / (f - n), 1.0f}}};

        int batch_count = sbcount(ctx->sb_draw_batches);
        for (int i = 0; i < batch_count; i++) {
            _DrawBatch *batch = &ctx->sb_draw_batches[i];
            if (batch->type == DRAW_BATCH_TYPE_2D) {
                _ext_dc_draw->submit_2d_layer(batch->draw_list_2d.layer);
                _ext_dc_draw_backend->submit_2d_drawlist(
                    batch->draw_list_2d.draw_list,
                    encoder,
                    draw_submit);
            } else if (batch->type == DRAW_BATCH_TYPE_3D && batch->draw_list_3d) {
                _ext_dc_draw_backend->submit_3d_drawlist(
                    batch->draw_list_3d,
                    encoder,
                    draw_submit,
                    &ortho_proj,
                    DC_DRAW_FLAG_DEPTH_TEST | DC_DRAW_FLAG_DEPTH_WRITE);
            }
        }
    }
}

void dc_app_draw_context_commit(DcAppDrawContext *ctx) {
    if (!ctx) return;

    ctx->pressed_target = ctx->next_pressed_target;
    ctx->hovered_target = ctx->next_hovered_target;
    if (ctx->screen_mouse.pressed) {
        ctx->active_target = ctx->next_pressed_target;
    }
    if (ctx->screen_mouse.released) {
        ctx->released_target = ctx->active_target;
        ctx->active_target   = (_DcAppDrawMouseTarget){0};
    } else {
        ctx->released_target = (_DcAppDrawMouseTarget){0};
    }
    ctx->next_hovered_target = (_DcAppDrawMouseTarget){0};
    ctx->next_pressed_target = (_DcAppDrawMouseTarget){0};
}

void dc_app_draw_context_push(DcAppDrawContext *ctx, plVec2 position, plVec2 dimensions, const plMat4 *transform) {
    if (!ctx) return;

    sbpush(ctx->sb_container_stack, ctx->area);
    ctx->area.position[0]   = position.x;
    ctx->area.position[1]   = position.y;
    ctx->area.dimensions[0] = dimensions.x;
    ctx->area.dimensions[1] = dimensions.y;

    plMat4 resolved_transform = transform ? *transform : pl_identity_mat4();
    memcpy(ctx->area.transform, resolved_transform.d, sizeof(ctx->area.transform));
    _draw_context_update_mouse(ctx);
}

void dc_app_draw_context_pop(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->sb_container_stack) == 0) return;
    if (sbcount(ctx->sb_scope_stack) > 0) {
        DcAppDrawScope *scope = &ctx->sb_scope_stack[sbcount(ctx->sb_scope_stack) - 1];
        if (sbcount(ctx->sb_container_stack) <= scope->container_count) return;
    }

    ctx->area = sbpop(ctx->sb_container_stack);
    _draw_context_update_mouse(ctx);
}

// A scope snapshots every mutable draw stack so nested callbacks cannot leak state.
DcAppDrawScope dc_app_draw_scope_begin(DcAppDrawContext *ctx) {
    if (!ctx) return (DcAppDrawScope){0};

    DcAppDrawScope scope = {
        .area                   = ctx->area,
        .container_count        = sbcount(ctx->sb_container_stack),
        .stencil_count          = sbcount(ctx->stencil.sb_frames),
        .planet_view_count      = sbcount(ctx->sb_planet_views),
        .planet_container_count = sbcount(ctx->sb_planet_container_stack),
    };
    sbpush(ctx->sb_scope_stack, scope);
    return scope;
}

void dc_app_draw_scope_end(DcAppDrawContext *ctx, DcAppDrawScope scope) {
    if (!ctx) return;

    if (sbcount(ctx->sb_scope_stack) > 0) {
        scope = ctx->sb_scope_stack[sbcount(ctx->sb_scope_stack) - 1];
    }

    // flushes queued planet views after overlays have been submitted.
    _flush_planet_views(ctx, scope.planet_view_count);

    while (sbcount(ctx->stencil.sb_frames) > scope.stencil_count) {
        dc_app_draw_stencil_end(ctx);
    }
    while (sbcount(ctx->sb_planet_container_stack) > scope.planet_container_count) {
        sbpop(ctx->sb_planet_container_stack);
    }
    while (sbcount(ctx->sb_container_stack) > scope.container_count) {
        sbpop(ctx->sb_container_stack);
    }
    if (sbcount(ctx->sb_scope_stack) > 0) {
        sbpop(ctx->sb_scope_stack);
    }

    ctx->area = scope.area;
    _draw_context_update_mouse(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] DrawFunction utility API
//-----------------------------------------------------------------------------

const DcAppDrawArea *dc_app_draw_get_area(DcAppDrawContext *ctx) {
    return ctx ? &ctx->area : NULL;
}

void dc_app_draw_resolve_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement, plVec2 *out, DcAppDrawArea *out_area) {
    if (!ctx || !points || point_count == 0 || !out) return;

    const DcAppDrawArea *base_area = &ctx->area;

    plMat4 transform = pl_identity_mat4();
    memcpy(transform.d, base_area->transform, sizeof(transform.d));

    float min_x = points[0].x;
    float min_y = points[0].y;
    float max_x = points[0].x;
    float max_y = points[0].y;
    for (uint32_t ii = 1; ii < point_count; ii++) {
        min_x = fminf(min_x, points[ii].x);
        min_y = fminf(min_y, points[ii].y);
        max_x = fmaxf(max_x, points[ii].x);
        max_y = fmaxf(max_y, points[ii].y);
    }

    if (_placement_is_default(placement) && position.x == 0.0f && position.y == 0.0f) {
        for (uint32_t ii = 0; ii < point_count; ii++) {
            plVec4 p = pl_mul_mat4_vec4(&transform, (plVec4){points[ii].x, points[ii].y, 0.0f, 1.0f});
            out[ii] = (plVec2){p.x, p.y};
        }
        if (out_area) {
            DcAppVec2 area_points[3] = {
                {min_x, min_y},
                {max_x, min_y},
                {min_x, max_y},
            };
            plVec2 resolved_area[3];
            dc_app_draw_resolve_points(ctx, area_points, 3, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, resolved_area, NULL);
            _draw_area_from_rect_points(max_x - min_x, max_y - min_y, resolved_area[0], resolved_area[1], resolved_area[2], out_area);
        }
        return;
    }

    float width  = max_x - min_x;
    float height = max_y - min_y;
    float x      = position.x;
    float y      = position.y;

    switch (placement.parent_align_x) {
        case DC_APP_ALIGN_TYPE_CENTER: x += base_area->dimensions[0] * 0.5f; break;
        case DC_APP_ALIGN_TYPE_RIGHT:  x += base_area->dimensions[0]; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_LEFT:
        default: break;
    }
    switch (placement.parent_align_y) {
        case DC_APP_ALIGN_TYPE_MIDDLE: y += base_area->dimensions[1] * 0.5f; break;
        case DC_APP_ALIGN_TYPE_TOP:    y += base_area->dimensions[1]; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_BOTTOM:
        default: break;
    }

    switch (placement.local_align_x) {
        case DC_APP_ALIGN_TYPE_CENTER: x -= width * 0.5f; break;
        case DC_APP_ALIGN_TYPE_RIGHT:  x -= width; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_LEFT:
        default: break;
    }
    switch (placement.local_align_y) {
        case DC_APP_ALIGN_TYPE_MIDDLE: y -= height * 0.5f; break;
        case DC_APP_ALIGN_TYPE_TOP:    y -= height; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_BOTTOM:
        default: break;
    }

    plVec2 pivot = {x, y};
    switch (placement.pivot_align_x) {
        case DC_APP_ALIGN_TYPE_CENTER: pivot.x += width * 0.5f; break;
        case DC_APP_ALIGN_TYPE_RIGHT:  pivot.x += width; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_LEFT:
        default: break;
    }
    switch (placement.pivot_align_y) {
        case DC_APP_ALIGN_TYPE_MIDDLE: pivot.y += height * 0.5f; break;
        case DC_APP_ALIGN_TYPE_TOP:    pivot.y += height; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_BOTTOM:
        default: break;
    }
    pivot.x += placement.pivot_x;
    pivot.y += placement.pivot_y;

    float rotation = pl_radiansf(placement.rotation);
    if (rotation != 0.0f) {
        float s = sinf(rotation);
        float c = cosf(rotation);
        for (uint32_t ii = 0; ii < point_count; ii++) {
            float placed_x = x + points[ii].x - min_x;
            float placed_y = y + points[ii].y - min_y;
            float px = placed_x - pivot.x;
            float py = placed_y - pivot.y;
            plVec4 p = pl_mul_mat4_vec4(&transform, (plVec4){pivot.x + px * c - py * s, pivot.y + px * s + py * c, 0.0f, 1.0f});
            out[ii] = (plVec2){p.x, p.y};
        }
    } else {
        for (uint32_t ii = 0; ii < point_count; ii++) {
            plVec4 p = pl_mul_mat4_vec4(&transform, (plVec4){x + points[ii].x - min_x, y + points[ii].y - min_y, 0.0f, 1.0f});
            out[ii] = (plVec2){p.x, p.y};
        }
    }

    if (out_area) {
        DcAppVec2 area_points[3] = {
            {min_x, min_y},
            {max_x, min_y},
            {min_x, max_y},
        };
        plVec2 resolved_area[3];
        dc_app_draw_resolve_points(ctx, area_points, 3, position, placement, resolved_area, NULL);
        _draw_area_from_rect_points(width, height, resolved_area[0], resolved_area[1], resolved_area[2], out_area);
    }
}

static plVec2 *_alloc_resolved_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area) {
    if (!ctx || !points || point_count == 0 || point_count > DCAPP_DRAW_POINT_COUNT_MAX) return NULL;
    if ((size_t)point_count > ((size_t)-1) / sizeof(plVec2)) return NULL;

    plVec2 *draw_points = PL_ALLOC(sizeof(plVec2) * point_count);
    if (!draw_points) return NULL;

    dc_app_draw_resolve_points(ctx, points, point_count, position, placement, draw_points, out_area);
    return draw_points;
}

static void _resolve_rect_points(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 out[4], DcAppDrawArea *out_area) {
    if (!ctx || !out) return;
    DcAppVec2 points[4] = {
        {0.0f, 0.0f},
        {dimensions.x, 0.0f},
        {dimensions.x, dimensions.y},
        {0.0f, dimensions.y},
    };
    dc_app_draw_resolve_points(ctx, points, 4, position, placement, out, NULL);
    _draw_area_from_rect_points(dimensions.x, dimensions.y, out[0], out[1], out[3], out_area);
}

static bool _placement_is_default(DcAppPlacement placement) {
    return placement.rotation == 0.0f &&
           placement.parent_align_x == DC_APP_ALIGN_TYPE_UNDEFINED &&
           placement.parent_align_y == DC_APP_ALIGN_TYPE_UNDEFINED &&
           placement.local_align_x == DC_APP_ALIGN_TYPE_UNDEFINED &&
           placement.local_align_y == DC_APP_ALIGN_TYPE_UNDEFINED &&
           placement.pivot_align_x == DC_APP_ALIGN_TYPE_UNDEFINED &&
           placement.pivot_align_y == DC_APP_ALIGN_TYPE_UNDEFINED &&
           placement.pivot_x == 0.0f &&
           placement.pivot_y == 0.0f;
}

static DcAppDrawArea *_draw_result_area(DcAppDrawResult *result) {
    return result ? &result->area : NULL;
}

static void _draw_context_update_mouse(DcAppDrawContext *ctx) {
    if (!ctx) return;

    ctx->mouse = ctx->screen_mouse;

    plMat4 transform = pl_identity_mat4();
    memcpy(transform.d, ctx->area.transform, sizeof(transform.d));

    plMat4 inv_transform = pl_mat4t_invert(&transform);
    plVec4 mouse_screen  = {
        ctx->screen_mouse.x,
        ctx->screen_mouse.y,
        0.0f,
        1.0f,
    };
    plVec4 mouse_local = pl_mul_mat4_vec4(&inv_transform, mouse_screen);

    ctx->mouse.x = mouse_local.x;
    ctx->mouse.y = mouse_local.y;
}

static void _draw_area_from_rect_points(float width, float height, plVec2 p0, plVec2 p1, plVec2 p3, DcAppDrawArea *out_area) {
    if (!out_area) return;

    *out_area = (DcAppDrawArea){0};
    out_area->position[0]   = p0.x;
    out_area->position[1]   = p0.y;
    out_area->dimensions[0] = width;
    out_area->dimensions[1] = height;

    plMat4 transform = pl_identity_mat4();
    if (width != 0.0f) {
        transform.x11 = (p1.x - p0.x) / width;
        transform.x21 = (p1.y - p0.y) / width;
    }
    if (height != 0.0f) {
        transform.x12 = (p3.x - p0.x) / height;
        transform.x22 = (p3.y - p0.y) / height;
    }
    transform.x14 = p0.x;
    transform.x24 = p0.y;

    memcpy(out_area->transform, transform.d, sizeof(out_area->transform));
}

static uint64_t _mouse_id(const char *id) {
    if (!id || id[0] == '\0') return 0;

    uint64_t hash = 0;
    for (const unsigned char *c = (const unsigned char *)id; *c; c++) {
        hash = *c + (hash << 6) + (hash << 16) - hash;
    }
    return hash ? hash : 1;
}

static _DcAppDrawMouseTarget _draw_mouse_target_internal(DcAppDrawTargetId target_id) {
    return (_DcAppDrawMouseTarget){
        .type   = _DC_APP_DRAW_MOUSE_TARGET_INTERNAL,
        .target = target_id,
    };
}

static _DcAppDrawMouseTarget _draw_mouse_target_id(uint64_t id) {
    return (_DcAppDrawMouseTarget){
        .type = _DC_APP_DRAW_MOUSE_TARGET_ID,
        .id   = id,
    };
}

static bool _draw_mouse_target_is_internal(_DcAppDrawMouseTarget target, DcAppDrawTargetId target_id) {
    return target.type == _DC_APP_DRAW_MOUSE_TARGET_INTERNAL && target.target == target_id;
}

static bool _draw_mouse_target_is_id(_DcAppDrawMouseTarget target, uint64_t id) {
    return target.type == _DC_APP_DRAW_MOUSE_TARGET_ID && target.id == id;
}

static bool _mouse_rect_local(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 *out) {
    if (!ctx || !out || dimensions.x == 0.0f || dimensions.y == 0.0f) return false;
    if (!ctx->screen_mouse.position_valid) return false;

    plVec2 points[4];
    _resolve_rect_points(ctx, dimensions, position, placement, points, NULL);

    plVec2 p0 = points[0];
    plVec2 p1 = points[1];
    plVec2 p3 = points[3];
    plVec2 vx = {p1.x - p0.x, p1.y - p0.y};
    plVec2 vy = {p3.x - p0.x, p3.y - p0.y};
    plVec2 pm = {ctx->screen_mouse.x - p0.x, ctx->screen_mouse.y - p0.y};

    float det = vx.x * vy.y - vx.y * vy.x;
    if (fabsf(det) <= 1e-8f) return false;

    float u = (pm.x * vy.y - pm.y * vy.x) / det;
    float v = (vx.x * pm.y - vx.y * pm.x) / det;
    out->x = u * dimensions.x;
    out->y = v * dimensions.y;
    return true;
}

static bool _mouse_point_in_polygon(plVec2 mouse, const plVec2 *points, uint32_t point_count) {
    bool inside = false;
    for (uint32_t ii = 0, jj = point_count - 1; ii < point_count; jj = ii++) {
        float xi = points[ii].x;
        float yi = points[ii].y;
        float xj = points[jj].x;
        float yj = points[jj].y;
        bool intersect = ((yi > mouse.y) != (yj > mouse.y)) && (mouse.x < (xj - xi) * (mouse.y - yi) / (yj - yi + 1e-12f) + xi);
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

static void _mouse_register(DcAppDrawContext *ctx, uint64_t id) {
    if (!ctx || id == 0) return;

    ctx->next_hovered_target = _draw_mouse_target_id(id);
    if (ctx->screen_mouse.pressed) {
        ctx->next_pressed_target = _draw_mouse_target_id(id);
    }
}

static dcDrawTextOptions _text_options(DcAppDrawContext *ctx, DcAppTextStyle style) {
    dcDrawTextOptions options = {0};

    options.ptFont = ctx ? ctx->default_font : NULL;
    options.fSize  = style.size;
    options.fWrap  = style.wrap;

    options.uColor = PL_COLOR_32_RGBA(style.color.r, style.color.g, style.color.b, style.color.a);

    return options;
}

static plMat3 _text_transform(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement) {
    const DcAppDrawArea *base_area = ctx ? &ctx->area : NULL;

    plMat4 transform = pl_identity_mat4();

    float align_x = 0.0f;
    switch (placement.local_align_x) {
        case DC_APP_ALIGN_TYPE_CENTER: align_x = -dimensions.x * 0.5f; break;
        case DC_APP_ALIGN_TYPE_RIGHT:  align_x = -dimensions.x; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_LEFT:
        default: break;
    }
    float align_y = 0.0f;
    switch (placement.local_align_y) {
        case DC_APP_ALIGN_TYPE_MIDDLE: align_y = -dimensions.y * 0.5f; break;
        case DC_APP_ALIGN_TYPE_TOP:    align_y = -dimensions.y; break;
        case DC_APP_ALIGN_TYPE_UNDEFINED:
        case DC_APP_ALIGN_TYPE_BOTTOM:
        default: break;
    }
    plMat4 trans_local_align_xform = pl_mat4_translate_xyz(align_x, align_y, 0.0f);
    transform = pl_mul_mat4t(&transform, &trans_local_align_xform);

    float anchor_x = 0.0f;
    float anchor_y = 0.0f;
    if (base_area) {
        switch (placement.parent_align_x) {
            case DC_APP_ALIGN_TYPE_CENTER: anchor_x = base_area->dimensions[0] * 0.5f; break;
            case DC_APP_ALIGN_TYPE_RIGHT:  anchor_x = base_area->dimensions[0]; break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
            default: break;
        }
        switch (placement.parent_align_y) {
            case DC_APP_ALIGN_TYPE_MIDDLE: anchor_y = base_area->dimensions[1] * 0.5f; break;
            case DC_APP_ALIGN_TYPE_TOP:    anchor_y = base_area->dimensions[1]; break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
            default: break;
        }
    }
    plMat4 trans_position_xform = pl_mat4_translate_xyz(anchor_x + position.x, anchor_y + position.y, 0.0f);
    transform = pl_mul_mat4t(&transform, &trans_position_xform);

    if (placement.rotation != 0.0f) {
        float pivot_x = 0.0f;
        float pivot_y = 0.0f;
        switch (placement.pivot_align_x) {
            case DC_APP_ALIGN_TYPE_CENTER: pivot_x = dimensions.x * 0.5f; break;
            case DC_APP_ALIGN_TYPE_RIGHT:  pivot_x = dimensions.x; break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
            default: break;
        }
        switch (placement.pivot_align_y) {
            case DC_APP_ALIGN_TYPE_MIDDLE: pivot_y = dimensions.y * 0.5f; break;
            case DC_APP_ALIGN_TYPE_TOP:    pivot_y = dimensions.y; break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
            default: break;
        }
        pivot_x += placement.pivot_x;
        pivot_y += placement.pivot_y;

        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_x, pivot_y, 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(pl_radiansf(placement.rotation), (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-pivot_x, -pivot_y, 0.0f);

        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    }

    plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0.0f, dimensions.y, 0.0f);
    plMat4 scale_invert_y_xform  = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);
    transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
    transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);

    if (base_area) {
        plMat4 base_transform = pl_identity_mat4();
        memcpy(base_transform.d, base_area->transform, sizeof(base_transform.d));
        transform = pl_mul_mat4t(&base_transform, &transform);
    }

    plMat3 transform3 = {0};
    transform3.x11    = transform.x11;
    transform3.x12    = transform.x12;
    transform3.x13    = transform.x14;
    transform3.x21    = transform.x21;
    transform3.x22    = transform.x22;
    transform3.x23    = transform.x24;
    transform3.x31    = transform.x31;
    transform3.x32    = transform.x32;
    transform3.x33    = transform.x33;
    return transform3;
}

bool dc_app_draw_container_push(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size) {
    return dc_app_draw_container_push_ex(ctx, position, size, virtual_size, (DcAppPlacement){0}, NULL);
}

bool dc_app_draw_container_push_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || size.x == 0.0f || size.y == 0.0f) return false;
    DcAppDrawArea *out_area = _draw_result_area(result);

    if (virtual_size.x == 0.0f) {
        virtual_size.x = size.x;
    }
    if (virtual_size.y == 0.0f) {
        virtual_size.y = size.y;
    }
    if (virtual_size.x == 0.0f || virtual_size.y == 0.0f) return false;

    sbpush(ctx->sb_container_stack, ctx->area);

    plVec2 points[4];
    _resolve_rect_points(ctx, size, position, placement, points, NULL);

    plVec2 p0 = points[0];
    plVec2 p1 = points[1];
    plVec2 p3 = points[3];

    plMat4 transform = pl_identity_mat4();
    transform.x11 = (p1.x - p0.x) / virtual_size.x;
    transform.x21 = (p1.y - p0.y) / virtual_size.x;
    transform.x12 = (p3.x - p0.x) / virtual_size.y;
    transform.x22 = (p3.y - p0.y) / virtual_size.y;
    transform.x14 = p0.x;
    transform.x24 = p0.y;

    ctx->area.position[0]   = 0.0f;
    ctx->area.position[1]   = 0.0f;
    ctx->area.dimensions[0] = virtual_size.x;
    ctx->area.dimensions[1] = virtual_size.y;
    memcpy(ctx->area.transform, transform.d, sizeof(ctx->area.transform));

    _draw_context_update_mouse(ctx);
    if (out_area) *out_area = ctx->area;

    return true;
}

bool dc_app_draw_container_push_area(DcAppDrawContext *ctx, const DcAppDrawArea *area) {
    if (!ctx || !area) return false;

    sbpush(ctx->sb_container_stack, ctx->area);

    ctx->area = *area;
    _draw_context_update_mouse(ctx);
    return true;
}

void dc_app_draw_container_pop(DcAppDrawContext *ctx) {
    dc_app_draw_context_pop(ctx);
}

bool dc_app_draw_stencil_begin(DcAppDrawContext *ctx) {
    if (!ctx) return false;

    int depth = sbcount(ctx->stencil.sb_frames) + 1;
    if (depth > DC_DRAW_STENCIL_MAX_DEPTH) {
        DC_LOG_WARN("Stencil", "Stencil nesting depth %d exceeds the %d available stencil bits; skipping stencil scope",
            depth,
            DC_DRAW_STENCIL_MAX_DEPTH);
        return false;
    }

    _DcAppStencilFrame frame = {
        .previous_phase = ctx->stencil.phase,
    };
    sbpush(ctx->stencil.sb_frames, frame);
    _set_stencil_phase(ctx, _DC_APP_STENCIL_PHASE_CREATE);
    return true;
}

void dc_app_draw_stencil_add(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->stencil.sb_frames) == 0) return;
    _set_stencil_phase(ctx, _DC_APP_STENCIL_PHASE_CREATE);
}

void dc_app_draw_stencil_remove(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->stencil.sb_frames) == 0) return;
    _set_stencil_phase(ctx, _DC_APP_STENCIL_PHASE_REMOVE);
}

void dc_app_draw_stencil_draw(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->stencil.sb_frames) == 0) return;
    _set_stencil_phase(ctx, _DC_APP_STENCIL_PHASE_DRAW);
}

void dc_app_draw_stencil_end(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->stencil.sb_frames) == 0) return;
    if (sbcount(ctx->sb_scope_stack) > 0) {
        DcAppDrawScope *scope = &ctx->sb_scope_stack[sbcount(ctx->sb_scope_stack) - 1];
        if (sbcount(ctx->stencil.sb_frames) <= scope->stencil_count) return;
    }

    _DcAppStencilFrame frame = ctx->stencil.sb_frames[sbcount(ctx->stencil.sb_frames) - 1];
    _set_stencil_phase(ctx, _DC_APP_STENCIL_PHASE_CLEANUP);
    _clear_stencil_bit(ctx);
    sbpop(ctx->stencil.sb_frames);
    _restore_stencil_phase(ctx, frame.previous_phase);
}

//-----------------------------------------------------------------------------
// [SECTION] DrawFunction mouse API
//-----------------------------------------------------------------------------

const DcAppMouse *dc_app_mouse_get_state(DcAppDrawContext *ctx) {
    return ctx ? &ctx->mouse : NULL;
}

const DcAppMouse *dc_app_draw_context_get_screen_mouse(DcAppDrawContext *ctx) {
    return ctx ? &ctx->screen_mouse : NULL;
}

void dc_app_draw_mouse_register_target(DcAppDrawContext *ctx, DcAppDrawTargetId target_id) {
    if (!ctx || target_id == 0) return;

    ctx->next_hovered_target = _draw_mouse_target_internal(target_id);
    if (ctx->screen_mouse.pressed) {
        ctx->next_pressed_target = _draw_mouse_target_internal(target_id);
    }
}

bool dc_app_draw_mouse_target_hovered(DcAppDrawContext *ctx, DcAppDrawTargetId target_id) {
    return ctx && target_id != 0 && _draw_mouse_target_is_internal(ctx->hovered_target, target_id);
}

bool dc_app_draw_mouse_target_pressed(DcAppDrawContext *ctx, DcAppDrawTargetId target_id) {
    return ctx && target_id != 0 && _draw_mouse_target_is_internal(ctx->pressed_target, target_id);
}

bool dc_app_draw_mouse_target_released(DcAppDrawContext *ctx, DcAppDrawTargetId target_id) {
    return ctx && target_id != 0 && _draw_mouse_target_is_internal(ctx->released_target, target_id);
}

bool dc_app_draw_mouse_target_active(DcAppDrawContext *ctx, DcAppDrawTargetId target_id) {
    return ctx && target_id != 0 && _draw_mouse_target_is_internal(ctx->active_target, target_id);
}

void dc_app_mouse_rect(DcAppDrawContext *ctx, const char *id, DcAppVec2 position, DcAppVec2 size) {
    dc_app_mouse_rect_ex(ctx, id, position, size, (DcAppPlacement){0});
}

void dc_app_mouse_rect_ex(DcAppDrawContext *ctx, const char *id, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement) {
    uint64_t mouse_id = _mouse_id(id);
    if (mouse_id == 0) return;

    plVec2 mouse_local;
    if (!_mouse_rect_local(ctx, size, position, placement, &mouse_local)) return;

    if (mouse_local.x > 0.0f && mouse_local.x < size.x && mouse_local.y > 0.0f && mouse_local.y < size.y) {
        _mouse_register(ctx, mouse_id);
    }
}

void dc_app_mouse_circle(DcAppDrawContext *ctx, const char *id, DcAppVec2 center, float radius) {
    dc_app_mouse_circle_ex(ctx, id, center, radius, (DcAppPlacement){0});
}

void dc_app_mouse_circle_ex(DcAppDrawContext *ctx, const char *id, DcAppVec2 center, float radius, DcAppPlacement placement) {
    dc_app_mouse_ellipse_ex(ctx, id, center, (DcAppVec2){radius, radius}, placement);
}

void dc_app_mouse_ellipse(DcAppDrawContext *ctx, const char *id, DcAppVec2 center, DcAppVec2 radius) {
    dc_app_mouse_ellipse_ex(ctx, id, center, radius, (DcAppPlacement){0});
}

void dc_app_mouse_ellipse_ex(DcAppDrawContext *ctx, const char *id, DcAppVec2 center, DcAppVec2 radius, DcAppPlacement placement) {
    uint64_t mouse_id = _mouse_id(id);
    if (mouse_id == 0 || radius.x <= 0.0f || radius.y <= 0.0f) return;

    if (placement.local_align_x == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_x = DC_APP_ALIGN_TYPE_CENTER;
    if (placement.local_align_y == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_y = DC_APP_ALIGN_TYPE_MIDDLE;

    DcAppVec2 diameter = {radius.x * 2.0f, radius.y * 2.0f};
    plVec2 mouse_local;
    if (!_mouse_rect_local(ctx, diameter, center, placement, &mouse_local)) return;

    float dx = mouse_local.x - radius.x;
    float dy = mouse_local.y - radius.y;
    if ((dx * dx) / (radius.x * radius.x) + (dy * dy) / (radius.y * radius.y) <= 1.0f) {
        _mouse_register(ctx, mouse_id);
    }
}

void dc_app_mouse_polygon(DcAppDrawContext *ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position) {
    dc_app_mouse_polygon_ex(ctx, id, points, point_count, position, (DcAppPlacement){0});
}

void dc_app_mouse_polygon_ex(DcAppDrawContext *ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement) {
    uint64_t mouse_id = _mouse_id(id);
    if (!ctx || mouse_id == 0 || !points || point_count < 3) return;
    if (!ctx->screen_mouse.position_valid) return;

    plVec2 *resolved = _alloc_resolved_points(ctx, points, point_count, position, placement, NULL);
    if (!resolved) return;

    bool inside = _mouse_point_in_polygon((plVec2){ctx->screen_mouse.x, ctx->screen_mouse.y}, resolved, point_count);
    PL_FREE(resolved);

    if (inside) {
        _mouse_register(ctx, mouse_id);
    }
}

bool dc_app_mouse_hovered(DcAppDrawContext *ctx, const char *id) {
    if (!ctx) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _draw_mouse_target_is_id(ctx->hovered_target, mouse_id);
}

bool dc_app_mouse_pressed(DcAppDrawContext *ctx, const char *id) {
    if (!ctx) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _draw_mouse_target_is_id(ctx->pressed_target, mouse_id);
}

bool dc_app_mouse_released(DcAppDrawContext *ctx, const char *id) {
    if (!ctx) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _draw_mouse_target_is_id(ctx->released_target, mouse_id);
}

bool dc_app_mouse_active(DcAppDrawContext *ctx, const char *id) {
    if (!ctx) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _draw_mouse_target_is_id(ctx->active_target, mouse_id);
}

bool dc_app_mouse_clicked(DcAppDrawContext *ctx, const char *id) {
    if (!ctx) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 &&
           _draw_mouse_target_is_id(ctx->released_target, mouse_id) &&
           _draw_mouse_target_is_id(ctx->hovered_target, mouse_id);
}

bool dc_app_mouse_down(DcAppDrawContext *ctx) {
    return ctx && ctx->mouse.down;
}

//-----------------------------------------------------------------------------
// [SECTION] DrawFunction primitive API
//-----------------------------------------------------------------------------

void dc_app_draw_line(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke) {
    dc_app_draw_line_ex(ctx, p0, p1, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_polyline(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke) {
    dc_app_draw_polyline_ex(ctx, points, point_count, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_polygon(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke) {
    dc_app_draw_polygon_ex(ctx, points, point_count, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_convex_polygon_filled(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color) {
    dc_app_draw_convex_polygon_filled_ex(ctx, points, point_count, color, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_polygon(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke) {
    dc_app_draw_rounded_polygon_ex(ctx, points, point_count, corner_radius, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_convex_polygon_filled(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color) {
    dc_app_draw_rounded_convex_polygon_filled_ex(ctx, points, point_count, corner_radius, color, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_quad(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke) {
    dc_app_draw_quad_ex(ctx, p0, p1, p2, p3, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_quad_filled(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color) {
    dc_app_draw_quad_filled_ex(ctx, p0, p1, p2, p3, color, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_quad(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke) {
    dc_app_draw_rounded_quad_ex(ctx, p0, p1, p2, p3, corner_radius, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_quad_filled(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color) {
    dc_app_draw_rounded_quad_filled_ex(ctx, p0, p1, p2, p3, corner_radius, color, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_image(DcAppDrawContext *ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint) {
    dc_app_draw_image_ex(ctx, texture_id, position, size, tint, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rect(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke) {
    dc_app_draw_rect_ex(ctx, position, size, stroke, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rect_filled(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color) {
    dc_app_draw_rect_filled_ex(ctx, position, size, color, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_rect(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke) {
    dc_app_draw_rounded_rect_ex(ctx, position, size, corner_radius, stroke, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_rect_filled(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color) {
    dc_app_draw_rounded_rect_filled_ex(ctx, position, size, corner_radius, color, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_circle(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppStroke stroke) {
    dc_app_draw_circle_ex(ctx, center, radius, stroke, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_circle_filled(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppVec4 color) {
    dc_app_draw_circle_filled_ex(ctx, center, radius, color, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_ellipse(DcAppDrawContext *ctx, DcAppVec2 center, DcAppVec2 radius, DcAppStroke stroke) {
    dc_app_draw_ellipse_ex(ctx, center, radius, stroke, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_ellipse_filled(DcAppDrawContext *ctx, DcAppVec2 center, DcAppVec2 radius, DcAppVec4 color) {
    dc_app_draw_ellipse_filled_ex(ctx, center, radius, color, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_text(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style) {
    dc_app_draw_text_ex(ctx, position, text, style, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_line_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    DcAppVec2 points[2] = {
        p0,
        p1,
    };
    dc_app_draw_polyline_ex(ctx, points, 2, stroke, position, placement, result);
}

void dc_app_draw_polyline_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !points || point_count < 2) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    _ext_dc_draw->add_lines(_draw_batch_get_2d(ctx), draw_points, point_count, (dcDrawLineOptions){
        .uColor       = PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        .fThickness   = stroke.width * DCAPP_LINE_WIDTH_FACTOR,
        .uDashPattern = stroke.pattern,
    });
    PL_FREE(draw_points);
}

void dc_app_draw_triangles_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !points || point_count < 3) return;
    if (point_count % 3 != 0) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    _ext_dc_draw->add_triangles_filled(_draw_batch_get_2d(ctx), draw_points, point_count / 3, (dcDrawSolidOptions){
        .uColor = PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
    });
    PL_FREE(draw_points);
}

void dc_app_draw_polygon_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_polygon_ex(ctx, points, point_count, 0.0f, stroke, position, placement, result);
}

void dc_app_draw_convex_polygon_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_convex_polygon_filled_ex(ctx, points, point_count, 0.0f, color, position, placement, result);
}

void dc_app_draw_rounded_polygon_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !points || point_count < 3) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    dcDrawLayer2D *layer = _draw_batch_get_2d(ctx);
    dcDrawLineOptions line_opts = {
        .uColor       = PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        .fThickness   = stroke.width * DCAPP_LINE_WIDTH_FACTOR,
        .uDashPattern = stroke.pattern,
    };
    if (corner_radius > 0.0f) {
        _ext_dc_draw->add_polygon_rounded(layer, draw_points, point_count, corner_radius, 8, line_opts);
    } else {
        _ext_dc_draw->add_polygon(layer, draw_points, point_count, line_opts);
    }
    PL_FREE(draw_points);
}

void dc_app_draw_rounded_convex_polygon_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !points || point_count < 3) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    dcDrawLayer2D *layer = _draw_batch_get_2d(ctx);
    if (corner_radius > 0.0f) {
        _ext_dc_draw->add_convex_polygon_rounded_filled(layer, draw_points, point_count, corner_radius, 8, (dcDrawSolidOptions){
            .uColor = PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        });
    } else {
        _ext_dc_draw->add_convex_polygon_filled(layer, draw_points, point_count, (dcDrawSolidOptions){
            .uColor = PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        });
    }
    PL_FREE(draw_points);
}

void dc_app_draw_quad_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_quad_ex(ctx, p0, p1, p2, p3, 0.0f, stroke, position, placement, result);
}

void dc_app_draw_quad_filled_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_quad_filled_ex(ctx, p0, p1, p2, p3, 0.0f, color, position, placement, result);
}

void dc_app_draw_rounded_quad_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    DcAppVec2 points[4] = {
        p0,
        p1,
        p2,
        p3,
    };
    dc_app_draw_rounded_polygon_ex(ctx, points, 4, corner_radius, stroke, position, placement, result);
}

void dc_app_draw_rounded_quad_filled_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    DcAppVec2 points[4] = {
        p0,
        p1,
        p2,
        p3,
    };
    dc_app_draw_rounded_convex_polygon_filled_ex(ctx, points, 4, corner_radius, color, position, placement, result);
}

void dc_app_draw_image_ex(DcAppDrawContext *ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    DcAppDrawArea *out_area = _draw_result_area(result);
    uint32_t bind_group_id = 0;
    if (!_resolve_texture_id(ctx, texture_id, &bind_group_id)) return;
    _draw_image_uv(ctx, bind_group_id, size,
                   (DcAppVec2){0.0f, 0.0f},
                   (DcAppVec2){0.0f, 1.0f},
                   (DcAppVec2){1.0f, 1.0f},
                   (DcAppVec2){1.0f, 0.0f},
                   position, placement, tint, out_area);
}

static void _draw_image_uv(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 dimensions, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area) {
    dc_app_draw_image_quad_uv(ctx, texture_id,
                              (DcAppVec2){0.0f, dimensions.y},
                              (DcAppVec2){0.0f, 0.0f},
                              (DcAppVec2){dimensions.x, 0.0f},
                              (DcAppVec2){dimensions.x, dimensions.y},
                              uv0, uv1, uv2, uv3, position, placement, tint, NULL);

    if (out_area) {
        plVec2 points[4];
        _resolve_rect_points(ctx, dimensions, position, placement, points, out_area);
    }
}

static bool _resolve_texture_id(DcAppDrawContext *ctx, DcAppTextureId texture_id, uint32_t *out) {
    if (out) *out = 0;
    if (!ctx || texture_id == 0 || !out) return false;
    return dc_app_texture_get_bind_group(ctx->texture_ctx, texture_id, out);
}

void dc_app_draw_image_quad(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area) {
    if (!ctx) return;

    DcAppVec2 points[4] = {p0, p1, p2, p3};
    plVec2 draw_points[4];
    dc_app_draw_resolve_points(ctx, points, 4, position, placement, draw_points, out_area);
    _ext_dc_draw->add_image_quad(_draw_batch_get_2d(ctx), texture_id, draw_points[0], draw_points[1], draw_points[2], draw_points[3]);
}

void dc_app_draw_image_quad_uv(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area) {
    if (!ctx) return;

    DcAppVec2 points[4] = {p0, p1, p2, p3};
    plVec2 draw_points[4];
    dc_app_draw_resolve_points(ctx, points, 4, position, placement, draw_points, out_area);
    _ext_dc_draw->add_image_quad_ex(_draw_batch_get_2d(ctx), texture_id, draw_points[0], draw_points[1], draw_points[2], draw_points[3],
                                    (plVec2){uv0.x, uv0.y}, (plVec2){uv1.x, uv1.y}, (plVec2){uv2.x, uv2.y}, (plVec2){uv3.x, uv3.y},
                                    PL_COLOR_32_RGBA(tint.r, tint.g, tint.b, tint.a));
}

void dc_app_draw_rect_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_rect_ex(ctx, position, size, 0.0f, stroke, placement, result);
}

void dc_app_draw_rect_filled_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_rect_filled_ex(ctx, position, size, 0.0f, color, placement, result);
}

void dc_app_draw_rounded_rect_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || size.x == 0.0f || size.y == 0.0f) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    dc_app_draw_rounded_quad_ex(ctx,
                                (DcAppVec2){0.0f, 0.0f},
                                (DcAppVec2){size.x, 0.0f},
                                (DcAppVec2){size.x, size.y},
                                (DcAppVec2){0.0f, size.y},
                                corner_radius, stroke, position, placement, NULL);

    if (out_area) {
        plVec2 points[4];
        _resolve_rect_points(ctx, size, position, placement, points, out_area);
    }
}

void dc_app_draw_rounded_rect_filled_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || size.x == 0.0f || size.y == 0.0f) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    dc_app_draw_rounded_quad_filled_ex(ctx,
                                       (DcAppVec2){0.0f, 0.0f},
                                       (DcAppVec2){size.x, 0.0f},
                                       (DcAppVec2){size.x, size.y},
                                       (DcAppVec2){0.0f, size.y},
                                       corner_radius, color, position, placement, NULL);

    if (out_area) {
        plVec2 points[4];
        _resolve_rect_points(ctx, size, position, placement, points, out_area);
    }
}

void dc_app_draw_circle_ex(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_ellipse_ex(ctx, center, (DcAppVec2){radius, radius}, stroke, placement, result);
}

void dc_app_draw_circle_filled_ex(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_ellipse_filled_ex(ctx, center, (DcAppVec2){radius, radius}, color, placement, result);
}

void dc_app_draw_ellipse_ex(DcAppDrawContext *ctx, DcAppVec2 center, DcAppVec2 radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || radius.x <= 0.0f || radius.y <= 0.0f) return;

    if (placement.local_align_x == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_x = DC_APP_ALIGN_TYPE_CENTER;
    if (placement.local_align_y == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_y = DC_APP_ALIGN_TYPE_MIDDLE;

    enum { SEGMENTS = 64 };
    DcAppVec2 points[SEGMENTS];

    for (int i = 0; i < SEGMENTS; i++) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)SEGMENTS;
        points[i] = (DcAppVec2){radius.x + cosf(theta) * radius.x, radius.y + sinf(theta) * radius.y};
    }

    dc_app_draw_polygon_ex(ctx, points, SEGMENTS, stroke, center, placement, result);
}

void dc_app_draw_ellipse_filled_ex(DcAppDrawContext *ctx, DcAppVec2 center, DcAppVec2 radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || radius.x <= 0.0f || radius.y <= 0.0f) return;

    if (placement.local_align_x == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_x = DC_APP_ALIGN_TYPE_CENTER;
    if (placement.local_align_y == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_y = DC_APP_ALIGN_TYPE_MIDDLE;

    enum { SEGMENTS = 64 };
    DcAppVec2 points[SEGMENTS];

    for (int i = 0; i < SEGMENTS; i++) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)SEGMENTS;
        points[i] = (DcAppVec2){radius.x + cosf(theta) * radius.x, radius.y + sinf(theta) * radius.y};
    }

    dc_app_draw_convex_polygon_filled_ex(ctx, points, SEGMENTS, color, center, placement, result);
}

DcAppVec2 dc_app_draw_text_size(DcAppDrawContext *ctx, const char *text, DcAppTextStyle style) {
    if (!ctx || !text || text[0] == '\0') return (DcAppVec2){0.0f, 0.0f};

    dcDrawTextOptions options = _text_options(ctx, style);
    plVec2 size = _ext_dc_draw->calculate_text_size(text, options);
    if (size.x > 0.0f && options.fSize > 0.0f && size.y < options.fSize) {
        size.y = options.fSize;
    }
    return (DcAppVec2){size.x, size.y};
}

void dc_app_draw_text_ex(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !text) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    DcAppVec2 size = dc_app_draw_text_size(ctx, text, style);
    if (size.x == 0.0f || size.y == 0.0f) return;

    plVec2 points[4];
    _resolve_rect_points(ctx, size, position, placement, points, out_area);

    dcDrawTextOptions options = _text_options(ctx, style);
    options.tTransform = _text_transform(ctx, size, position, placement);
    _ext_dc_draw->add_text(_draw_batch_get_2d(ctx), (plVec2){0.0f, 0.0f}, text, options);
}

//-----------------------------------------------------------------------------
// [SECTION] Internal XML/node draw helpers
//-----------------------------------------------------------------------------

plVec2 dc_app_draw_text_options_size(const char *text, dcDrawTextOptions options) {
    if (!text) return (plVec2){0.0f, 0.0f};
    return _ext_dc_draw->calculate_text_size(text, options);
}

void dc_app_draw_text_options(DcAppDrawContext *ctx, const char *text, dcDrawTextOptions options) {
    if (!ctx || !text) return;
    _ext_dc_draw->add_text(_draw_batch_get_2d(ctx), (plVec2){0.0f, 0.0f}, text, options);
}

void dc_app_draw_3d_sphere_textured(DcAppDrawContext *ctx, uint32_t texture_id, plSphere sphere, const plMat4 *transform, uint32_t color) {
    if (!ctx || !transform) return;
    _ext_dc_draw->add_3d_sphere_textured(_draw_batch_get_3d(ctx), texture_id, sphere, transform, 32, 32, color);
}

void dc_app_draw_3d_sphere_filled(DcAppDrawContext *ctx, plSphere sphere, uint32_t color) {
    if (!ctx) return;
    _ext_dc_draw->add_3d_sphere_filled(_draw_batch_get_3d(ctx), sphere, 32, 32, (dcDrawSolidOptions){.uColor = color});
}

void dc_app_draw_planet_convex_polygon_filled(plPlanetView *view, plVec3 *points, uint32_t point_count, uint32_t color) {
    if (!view || !points || point_count < 3) return;
    _ext_planet->draw_convex_polygon_filled(view, points, point_count, color);
}

void dc_app_draw_planet_polygon(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color, uint8_t line_pattern) {
    if (!view || !points || point_count < 3) return;
    _ext_planet->draw_polygon(view, points, point_count, line_width, color, line_pattern);
}

void dc_app_draw_planet_line(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color, uint8_t line_pattern) {
    if (!view || !points || point_count < 2) return;
    _ext_planet->draw_line(view, points, point_count, line_width, color, line_pattern);
}

void dc_app_draw_planet_ellipse(plPlanetView *view, plVec3 center, plVec2 radius, float rotation_degrees, uint32_t segments, float line_width, uint32_t line_color, bool line_enabled, uint32_t fill_color, bool fill_enabled) {
    if (!view || radius.x <= 0.0f || radius.y <= 0.0f) return;
    if (segments == 0) segments = 64;
    if (segments < 3) segments = 3;
    if (segments > DC_APP_PLANET_ELLIPSE_MAX_SEGMENTS) segments = DC_APP_PLANET_ELLIPSE_MAX_SEGMENTS;

    plVec3 up = pl_norm_vec3(center);
    plVec3 east = pl_cross_vec3((plVec3){0.0f, 1.0f, 0.0f}, up);
    if (pl_length_vec3(east) < 1e-6f) {
        east = (plVec3){1.0f, 0.0f, 0.0f};
    } else {
        east = pl_norm_vec3(east);
    }
    plVec3 north = pl_cross_vec3(up, east);
    float rotation = pl_radiansf(rotation_degrees);
    float rotation_cos = cosf(rotation);
    float rotation_sin = sinf(rotation);

    plVec3 points[DC_APP_PLANET_ELLIPSE_MAX_SEGMENTS];
    for (uint32_t i = 0; i < segments; i++) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)segments;
        float local_x = radius.x * cosf(theta);
        float local_y = radius.y * sinf(theta);
        float x = local_x * rotation_cos - local_y * rotation_sin;
        float y = local_x * rotation_sin + local_y * rotation_cos;
        points[i] = (plVec3){
            center.x + x * east.x + y * north.x,
            center.y + x * east.y + y * north.y,
            center.z + x * east.z + y * north.z,
        };
    }

    if (fill_enabled) dc_app_draw_planet_convex_polygon_filled(view, points, segments, fill_color);
    if (line_enabled) dc_app_draw_planet_polygon(view, points, segments, line_width, line_color, 0);
}

void dc_app_draw_planet_sphere(plPlanetView *view, float lon, float lat, float height, float radius, uint32_t color) {
    if (!view || radius <= 0.0f) return;
    _ext_planet->draw_sphere(view, lon, lat, height, radius, color);
}

void dc_app_draw_planet_text(plPlanetView *view, plCamera *camera, plVec3 position, const char *text, float size, uint32_t color) {
    if (!view || !camera || !text) return;
    _ext_planet->draw_text(view, camera, position, text, size, color);
}

DcAppDrawPlanetViewHandle dc_app_draw_planet_view_geodetic(DcAppDrawContext *ctx, DcAppPlanetViewHandle view, double lat, double lon, double elevation, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppPlanetViewOptions options, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !view || dc_app_planet_view_crs(view) != DC_APP_PLANET_CRS_GEODETIC) return NULL;

    DcAppPlanetHandle planet = dc_app_planet_view_planet(view);
    // stores draw-frame camera and placement until the current draw scope ends.
    DcAppDrawPlanetViewHandle draw_view = (DcAppDrawPlanetViewHandle)PL_ALLOC(sizeof(*draw_view));
    memset(draw_view, 0, sizeof(*draw_view));
    draw_view->view = view;
    draw_view->options = options;
    draw_view->logical_dimensions = (plVec2){size.x, size.y};

    plPlanetView *pl_view = dc_app_planet_view_pl(view);
    if (pl_view) {
        DcAppDrawResult image_result = {0};
        plBindGroupHandle bind_group = _ext_planet->get_view_texture(pl_view);
        _draw_image_uv(ctx, bind_group.uData, size,
                       (DcAppVec2){0.0f, 0.0f},
                       (DcAppVec2){0.0f, 1.0f},
                       (DcAppVec2){1.0f, 1.0f},
                       (DcAppVec2){1.0f, 0.0f},
                       position, placement,
                       (DcAppVec4){1.0f, 1.0f, 1.0f, 1.0f},
                       _draw_result_area(result ? result : &image_result));
        draw_view->area = result ? result->area : image_result.area;
        plMat4 transform;
        memcpy(transform.d, draw_view->area.transform, sizeof(transform.d));
        draw_view->logical_dimensions = (plVec2){
            hypotf(transform.x11 * draw_view->area.dimensions[0], transform.x21 * draw_view->area.dimensions[0]),
            hypotf(transform.x12 * draw_view->area.dimensions[1], transform.x22 * draw_view->area.dimensions[1]),
        };
    }

    draw_view->camera = _planet_camera_geodetic(
        planet, lat, lon, elevation, rpy, fov_degrees, orthographic,
        (DcAppVec2){draw_view->logical_dimensions.x, draw_view->logical_dimensions.y});
    _apply_planet_view_options(draw_view);
    sbpush(ctx->sb_planet_views, draw_view);
    return draw_view;
}

DcAppDrawPlanetViewHandle dc_app_draw_planet_view_cartesian(DcAppDrawContext *ctx, DcAppPlanetViewHandle view, DcAppVec3d camera_position, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppPlanetViewOptions options, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result) {
    if (result) *result = (DcAppDrawResult){0};
    if (!ctx || !view || dc_app_planet_view_crs(view) != DC_APP_PLANET_CRS_CARTESIAN) return NULL;

    DcAppPlanetHandle planet = dc_app_planet_view_planet(view);
    // stores draw-frame camera and placement until the current draw scope ends.
    DcAppDrawPlanetViewHandle draw_view = (DcAppDrawPlanetViewHandle)PL_ALLOC(sizeof(*draw_view));
    memset(draw_view, 0, sizeof(*draw_view));
    draw_view->view = view;
    draw_view->options = options;
    draw_view->logical_dimensions = (plVec2){size.x, size.y};

    plPlanetView *pl_view = dc_app_planet_view_pl(view);
    if (pl_view) {
        DcAppDrawResult image_result = {0};
        plBindGroupHandle bind_group = _ext_planet->get_view_texture(pl_view);
        _draw_image_uv(ctx, bind_group.uData, size,
                       (DcAppVec2){0.0f, 0.0f},
                       (DcAppVec2){0.0f, 1.0f},
                       (DcAppVec2){1.0f, 1.0f},
                       (DcAppVec2){1.0f, 0.0f},
                       position, placement,
                       (DcAppVec4){1.0f, 1.0f, 1.0f, 1.0f},
                       _draw_result_area(result ? result : &image_result));
        draw_view->area = result ? result->area : image_result.area;
        plMat4 transform;
        memcpy(transform.d, draw_view->area.transform, sizeof(transform.d));
        draw_view->logical_dimensions = (plVec2){
            hypotf(transform.x11 * draw_view->area.dimensions[0], transform.x21 * draw_view->area.dimensions[0]),
            hypotf(transform.x12 * draw_view->area.dimensions[1], transform.x22 * draw_view->area.dimensions[1]),
        };
    }

    draw_view->camera = _planet_camera_cartesian(
        planet, camera_position, rpy, fov_degrees, orthographic,
        (DcAppVec2){draw_view->logical_dimensions.x, draw_view->logical_dimensions.y});
    _apply_planet_view_options(draw_view);
    sbpush(ctx->sb_planet_views, draw_view);
    return draw_view;
}

bool dc_app_draw_planet_container_push_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, double lat, double lon, double height, DcAppPlanetLocalTransform transform) {
    if (!ctx || !draw_view || !draw_view->view) return false;
    if (sbcount(ctx->sb_planet_container_stack) > 0) {
        DC_LOG_WARN("PlanetContainer", "Nested planet containers are not supported");
        return false;
    }

    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    double planet_radius = dc_app_planet_radius(planet);
    if (planet_radius <= 0.0) return false;

    double lat_radians = lat * M_PI / 180.0;
    double lon_radians = lon * M_PI / 180.0;
    double rotation = (double)transform.rotation_degrees * M_PI / 180.0;
    double lat_cos = cos(lat_radians);
    double lat_sin = sin(lat_radians);
    double lon_cos = cos(lon_radians);
    double lon_sin = sin(lon_radians);

    _DcAppPlanetContainerFrame frame = {
        .draw_view = draw_view,
        .up = {lat_cos * lon_sin, lat_sin, lat_cos * lon_cos},
        .east = {lon_cos, 0.0, -lon_sin},
        .north = {-lat_sin * lon_sin, lat_cos, -lat_sin * lon_cos},
        .planet_radius = planet_radius,
        .surface_radius = planet_radius + height,
        .scale = transform.scale,
        .rotation_cos = cos(rotation),
        .rotation_sin = sin(rotation),
    };
    sbpush(ctx->sb_planet_container_stack, frame);
    return true;
}

void dc_app_draw_planet_container_pop(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->sb_planet_container_stack) == 0) return;
    if (sbcount(ctx->sb_scope_stack) > 0) {
        DcAppDrawScope *scope = &ctx->sb_scope_stack[sbcount(ctx->sb_scope_stack) - 1];
        if (sbcount(ctx->sb_planet_container_stack) <= scope->planet_container_count) return;
    }
    sbpop(ctx->sb_planet_container_stack);
}

void dc_app_draw_planet_line_local(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke) {
    if (!points || point_count < 2) return;

    _DcAppPlanetContainerFrame *frame = _planet_container_frame(ctx);
    plVec3 *cartesian = _planet_container_transform_points(ctx, points, point_count);
    if (!frame || !cartesian) return;

    dc_app_draw_planet_line(
        dc_app_planet_view_pl(frame->draw_view->view),
        cartesian,
        point_count,
        stroke.width,
        PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        stroke.pattern);
    PL_FREE(cartesian);
}

void dc_app_draw_planet_polygon_local(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke) {
    dc_app_draw_planet_polygon_local_enabled(
        ctx,
        points,
        point_count,
        stroke.width,
        PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        stroke.pattern,
        true,
        0,
        false);
}

void dc_app_draw_planet_convex_polygon_filled_local(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color) {
    dc_app_draw_planet_polygon_local_enabled(
        ctx,
        points,
        point_count,
        0.0f,
        0,
        0,
        false,
        PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        true);
}

void dc_app_draw_planet_text_local(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, float size, DcAppVec4 color) {
    if (!text) return;

    _DcAppPlanetContainerFrame *frame = _planet_container_frame(ctx);
    plVec3 *cartesian = _planet_container_transform_points(ctx, &position, 1);
    if (!frame || !cartesian) return;

    dc_app_draw_planet_text_cartesian(
        ctx,
        frame->draw_view,
        (DcAppVec3d){cartesian[0].x, cartesian[0].y, cartesian[0].z},
        text,
        size * (float)fabs(frame->scale),
        color);
    PL_FREE(cartesian);
}

void dc_app_draw_planet_polygon_local_enabled(
    DcAppDrawContext *ctx,
    const DcAppVec2 *points,
    uint32_t point_count,
    float line_width,
    uint32_t line_color,
    uint8_t line_pattern,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled) {
    if (!points || point_count < 3) return;

    _DcAppPlanetContainerFrame *frame = _planet_container_frame(ctx);
    plVec3 *cartesian = _planet_container_transform_points(ctx, points, point_count);
    if (!frame || !cartesian) return;

    plPlanetView *view = dc_app_planet_view_pl(frame->draw_view->view);
    if (fill_enabled)
        dc_app_draw_planet_convex_polygon_filled(view, cartesian, point_count, fill_color);
    if (line_enabled)
        dc_app_draw_planet_polygon(view, cartesian, point_count, line_width, line_color, line_pattern);
    PL_FREE(cartesian);
}

void dc_app_draw_planet_sphere_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, double lat, double lon, double height, double radius, DcAppVec4 color) {
    (void)ctx;
    if (!draw_view) return;
    dc_app_draw_planet_sphere(dc_app_planet_view_pl(draw_view->view), (float)lon, (float)lat, (float)height, (float)radius, PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a));
}

void dc_app_draw_planet_sphere_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec3d position, float radius, DcAppVec4 color) {
    (void)ctx;
    if (!draw_view) return;
    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    if (!cartesian_crs || !geodetic_crs) return;
    // converts cartesian centers because pl_planet draws spheres from geodetic centers.
    plVec3d cartesian = {position.x, position.y, position.z};
    plVec3d geodetic;
    dc_geo_cartesian_to_geodetic_d(cartesian_crs, geodetic_crs, &cartesian, &geodetic, 1);
    dc_app_draw_planet_sphere(dc_app_planet_view_pl(draw_view->view), (float)geodetic.y, (float)geodetic.x, (float)geodetic.z, radius, PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a));
}

void dc_app_draw_planet_line_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, const DcAppVec3d *points, uint32_t point_count, DcAppStroke stroke) {
    (void)ctx;
    if (!draw_view || !points || point_count < 2) return;

    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return;

    plVec3 *cartesian = (plVec3 *)PL_ALLOC(sizeof(plVec3) * point_count);
    if (!cartesian) return;

    for (uint32_t i = 0; i < point_count; i++) {
        plVec3d geodetic = {points[i].x, points[i].y, points[i].z};
        plVec3d converted;
        dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic, &converted, 1);
        cartesian[i] = (plVec3){(float)converted.x, (float)converted.y, (float)converted.z};
    }

    dc_app_draw_planet_line(
        dc_app_planet_view_pl(draw_view->view),
        cartesian,
        point_count,
        stroke.width,
        PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        stroke.pattern);
    PL_FREE(cartesian);
}

void dc_app_draw_planet_line_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, const DcAppVec3d *points, uint32_t point_count, DcAppStroke stroke) {
    (void)ctx;
    if (!draw_view || !points || point_count < 2) return;

    plVec3 *cartesian = (plVec3 *)PL_ALLOC(sizeof(plVec3) * point_count);
    if (!cartesian) return;

    for (uint32_t i = 0; i < point_count; i++) {
        cartesian[i] = (plVec3){(float)points[i].x, (float)points[i].y, (float)points[i].z};
    }

    dc_app_draw_planet_line(
        dc_app_planet_view_pl(draw_view->view),
        cartesian,
        point_count,
        stroke.width,
        PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        stroke.pattern);
    PL_FREE(cartesian);
}

static void _draw_planet_polygon_geodetic_enabled(
    DcAppDrawPlanetViewHandle draw_view,
    const DcAppVec3d *points,
    uint32_t point_count,
    float line_width,
    uint32_t line_color,
    uint8_t line_pattern,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled) {
    if (!draw_view || !points || point_count < 3) return;

    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return;

    plVec3 *cartesian = (plVec3 *)PL_ALLOC(sizeof(plVec3) * point_count);
    if (!cartesian) return;

    for (uint32_t i = 0; i < point_count; i++) {
        plVec3d geodetic = {points[i].x, points[i].y, points[i].z};
        plVec3d converted;
        dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic, &converted, 1);
        cartesian[i] = (plVec3){(float)converted.x, (float)converted.y, (float)converted.z};
    }

    plPlanetView *view = dc_app_planet_view_pl(draw_view->view);
    if (fill_enabled)
        dc_app_draw_planet_convex_polygon_filled(view, cartesian, point_count, fill_color);
    if (line_enabled)
        dc_app_draw_planet_polygon(view, cartesian, point_count, line_width, line_color, line_pattern);
    PL_FREE(cartesian);
}

void dc_app_draw_planet_polygon_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, const DcAppVec3d *points, uint32_t point_count, DcAppStroke stroke) {
    (void)ctx;
    _draw_planet_polygon_geodetic_enabled(
        draw_view,
        points,
        point_count,
        stroke.width,
        PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        stroke.pattern,
        true,
        0,
        false);
}

void dc_app_draw_planet_convex_polygon_filled_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, const DcAppVec3d *points, uint32_t point_count, DcAppVec4 color) {
    (void)ctx;
    _draw_planet_polygon_geodetic_enabled(
        draw_view,
        points,
        point_count,
        0.0f,
        0,
        0,
        false,
        PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        true);
}

void dc_app_draw_planet_polygon_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, const DcAppVec3d *points, uint32_t point_count, DcAppStroke stroke) {
    (void)ctx;
    dc_app_draw_planet_polygon_cartesian_enabled(
        draw_view,
        points,
        point_count,
        stroke.width,
        PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        stroke.pattern,
        true,
        0,
        false);
}

void dc_app_draw_planet_convex_polygon_filled_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, const DcAppVec3d *points, uint32_t point_count, DcAppVec4 color) {
    (void)ctx;
    dc_app_draw_planet_polygon_cartesian_enabled(
        draw_view,
        points,
        point_count,
        0.0f,
        0,
        0,
        false,
        PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        true);
}

void dc_app_draw_planet_polygon_cartesian_enabled(
    DcAppDrawPlanetViewHandle draw_view,
    const DcAppVec3d *points,
    uint32_t point_count,
    float line_width,
    uint32_t line_color,
    uint8_t line_pattern,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled) {
    if (!draw_view || !points || point_count < 3) return;

    plVec3 *cartesian = (plVec3 *)PL_ALLOC(sizeof(plVec3) * point_count);
    if (!cartesian) return;

    for (uint32_t i = 0; i < point_count; i++) {
        cartesian[i] = (plVec3){(float)points[i].x, (float)points[i].y, (float)points[i].z};
    }

    plPlanetView *view = dc_app_planet_view_pl(draw_view->view);
    if (fill_enabled)
        dc_app_draw_planet_convex_polygon_filled(view, cartesian, point_count, fill_color);
    if (line_enabled)
        dc_app_draw_planet_polygon(view, cartesian, point_count, line_width, line_color, line_pattern);
    PL_FREE(cartesian);
}

void dc_app_draw_planet_image_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, double lat, double lon, double height, DcAppTextureId texture_id, DcAppVec2 size, DcAppVec4 tint) {
    if (!ctx || !draw_view || !draw_view->view || texture_id == 0) return;
    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return;

    plVec3d geodetic_in = {lat, lon, height};
    plVec3d cartesian_out;
    dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic_in, &cartesian_out, 1);
    dc_app_draw_planet_image_cartesian(ctx, draw_view, (DcAppVec3d){cartesian_out.x, cartesian_out.y, cartesian_out.z}, texture_id, size, tint);
}

void dc_app_draw_planet_image_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec3d position, DcAppTextureId texture_id, DcAppVec2 size, DcAppVec4 tint) {
    if (!ctx || !draw_view || !draw_view->view || texture_id == 0) return;

    DcAppVec2 size_meters = _planet_image_size_meters(ctx, texture_id, size);
    if (size_meters.x <= 0.0f || size_meters.y <= 0.0f) return;

    DcAppVec2 image_position = {0};
    float image_width = 0.0f;
    if (!_planet_project_overlay(draw_view, position, size_meters.x, &image_position, &image_width)) return;

    float image_height = image_width * (size_meters.y / size_meters.x);
    _planet_draw_image_label(ctx, draw_view, texture_id, image_position, (DcAppVec2){image_width, image_height}, tint);
}

void dc_app_draw_planet_text_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, double lat, double lon, double height, const char *text, float size, DcAppVec4 color) {
    if (!ctx || !draw_view || !draw_view->view || !text) return;
    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return;
    // converts geodetic text positions to renderer-native cartesian coordinates.
    plVec3d geodetic_in = {lat, lon, height};
    plVec3d cartesian_out;
    dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic_in, &cartesian_out, 1);
    DcAppVec2 position = {0};
    float text_size = 0.0f;
    if (!_planet_project_overlay(draw_view, (DcAppVec3d){cartesian_out.x, cartesian_out.y, cartesian_out.z}, size, &position, &text_size)) return;
    _planet_draw_text_label(ctx, draw_view, position, text, text_size, color);
}

void dc_app_draw_planet_text_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec3d position, const char *text, float size, DcAppVec4 color) {
    if (!ctx || !draw_view || !draw_view->view || !text) return;
    DcAppVec2 text_position = {0};
    float text_size = 0.0f;
    if (!_planet_project_overlay(draw_view, position, size, &text_position, &text_size)) return;
    _planet_draw_text_label(ctx, draw_view, text_position, text, text_size, color);
}

void dc_app_draw_planet_ellipse_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, double lat, double lon, double height, DcAppVec2 radius, float rotation_degrees, uint32_t segments, float line_width, DcAppVec4 color) {
    if (!draw_view || !draw_view->view) return;
    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return;

    plVec3d geodetic = {lat, lon, height};
    plVec3d cartesian;
    dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic, &cartesian, 1);
    dc_app_draw_planet_ellipse_cartesian(
        ctx,
        draw_view,
        (DcAppVec3d){cartesian.x, cartesian.y, cartesian.z},
        radius,
        rotation_degrees,
        segments,
        line_width,
        color);
}

void dc_app_draw_planet_ellipse_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec3d center, DcAppVec2 radius, float rotation_degrees, uint32_t segments, float line_width, DcAppVec4 color) {
    (void)ctx;
    dc_app_draw_planet_ellipse_cartesian_enabled(
        draw_view,
        &center,
        &radius,
        rotation_degrees,
        segments,
        line_width,
        PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        true,
        0,
        false);
}

void dc_app_draw_planet_ellipse_filled_geodetic(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, double lat, double lon, double height, DcAppVec2 radius, float rotation_degrees, uint32_t segments, DcAppVec4 color) {
    if (!draw_view || !draw_view->view) return;
    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return;

    plVec3d geodetic = {lat, lon, height};
    plVec3d cartesian;
    dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic, &cartesian, 1);
    dc_app_draw_planet_ellipse_filled_cartesian(
        ctx,
        draw_view,
        (DcAppVec3d){cartesian.x, cartesian.y, cartesian.z},
        radius,
        rotation_degrees,
        segments,
        color);
}

void dc_app_draw_planet_ellipse_filled_cartesian(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec3d center, DcAppVec2 radius, float rotation_degrees, uint32_t segments, DcAppVec4 color) {
    (void)ctx;
    dc_app_draw_planet_ellipse_cartesian_enabled(
        draw_view,
        &center,
        &radius,
        rotation_degrees,
        segments,
        0.0f,
        0,
        false,
        PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
        true);
}

void dc_app_draw_planet_ellipse_cartesian_enabled(
    DcAppDrawPlanetViewHandle draw_view,
    const DcAppVec3d *center,
    const DcAppVec2 *radius,
    float rotation_degrees,
    uint32_t segments,
    float line_width,
    uint32_t line_color,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled) {
    if (!draw_view || !draw_view->view || !center || !radius) return;

    dc_app_draw_planet_ellipse(
        dc_app_planet_view_pl(draw_view->view),
        (plVec3){(float)center->x, (float)center->y, (float)center->z},
        (plVec2){radius->x, radius->y}, rotation_degrees, segments, line_width,
        line_color, line_enabled,
        fill_color, fill_enabled);
}

void dc_app_draw_planet_geojson(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppPlanetGeojsonHandle geojson, DcAppPlanetGeojsonStyle style) {
    if (!ctx || !draw_view || !draw_view->view || !geojson) return;

    DcGeojson *geojson_data = dc_app_planet_geojson(geojson);
    uint32_t feature_count = dc_geojson_feature_count(geojson_data);
    for (uint32_t i = 0; i < feature_count; i++) {
        const DcGeojsonFeature *feature = dc_geojson_feature(geojson_data, i);
        if (feature) _planet_draw_geojson_feature(draw_view, feature, _planet_geojson_style(feature, style));
    }
}

static _DcAppResolvedGeojsonStyle _planet_geojson_style(const DcGeojsonFeature *feature, DcAppPlanetGeojsonStyle fallback) {
    bool line_enabled = (fallback.flags & DC_APP_PLANET_GEOJSON_STYLE_FLAGS_LINE_COLOR) != 0;
    bool fill_enabled = (fallback.flags & DC_APP_PLANET_GEOJSON_STYLE_FLAGS_FILL_COLOR) != 0;
    bool line_width_set = (fallback.flags & DC_APP_PLANET_GEOJSON_STYLE_FLAGS_LINE_WIDTH) != 0;
    _DcAppResolvedGeojsonStyle style = {
        .height_above_terrain = fallback.height_above_terrain,
        .line_width = line_width_set ? fallback.line_width : 1.0f,
        .line_color = line_enabled ? fallback.line_color : (DcAppVec4){1.0f, 1.0f, 1.0f, 1.0f},
        .fill_color = fallback.fill_color,
        .line_enabled = line_enabled,
        .fill_enabled = fill_enabled,
    };

    if (feature->style.stroke.has_value) {
        style.line_color = (DcAppVec4){
            feature->style.stroke.r,
            feature->style.stroke.g,
            feature->style.stroke.b,
            feature->style.stroke.a,
        };
        style.line_enabled = true;
    }
    if (feature->style.fill.has_value) {
        style.fill_color = (DcAppVec4){
            feature->style.fill.r,
            feature->style.fill.g,
            feature->style.fill.b,
            feature->style.fill.a,
        };
        style.fill_enabled = true;
    }
    if (feature->style.has_stroke_width) {
        style.line_width = feature->style.stroke_width;
    }
    return style;
}

static plVec3 *_planet_geojson_points(DcAppPlanetHandle planet, const DcGeojsonCoordArray *coordinates, double height_above_terrain) {
    if (!planet || !coordinates || coordinates->count == 0) return NULL;

    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return NULL;

    plVec3 *points = (plVec3 *)PL_ALLOC(sizeof(plVec3) * coordinates->count);
    if (!points) return NULL;

    for (uint32_t i = 0; i < coordinates->count; i++) {
        const DcGeojsonPosition *position = &coordinates->positions[i];
        plVec3d geodetic = {
            position->lat,
            position->lon,
            position->has_alt ? position->alt : height_above_terrain,
        };
        plVec3d cartesian;
        dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic, &cartesian, 1);
        points[i] = (plVec3){(float)cartesian.x, (float)cartesian.y, (float)cartesian.z};
    }
    return points;
}

static void _planet_draw_geojson_feature(DcAppDrawPlanetViewHandle draw_view, const DcGeojsonFeature *feature, _DcAppResolvedGeojsonStyle style) {
    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    plPlanetView *view = dc_app_planet_view_pl(draw_view->view);
    if (!planet || !view || !feature) return;

    uint32_t line_color = PL_COLOR_32_RGBA(style.line_color.r, style.line_color.g, style.line_color.b, style.line_color.a);
    uint32_t fill_color = PL_COLOR_32_RGBA(style.fill_color.r, style.fill_color.g, style.fill_color.b, style.fill_color.a);

    switch (feature->type) {
        case DC_GEOJSON_FEATURE_POINT: {
            const DcGeojsonPosition *point = &feature->geom.point.position;
            dc_app_draw_planet_sphere(view, (float)point->lon, (float)point->lat,
                (float)(point->has_alt ? point->alt : style.height_above_terrain),
                1000.0f, line_color);
            break;
        }

        case DC_GEOJSON_FEATURE_MULTI_POINT: {
            for (uint32_t i = 0; i < feature->geom.multi_point.count; i++) {
                const DcGeojsonPosition *point = &feature->geom.multi_point.positions[i];
                dc_app_draw_planet_sphere(view, (float)point->lon, (float)point->lat,
                    (float)(point->has_alt ? point->alt : style.height_above_terrain),
                    1000.0f, line_color);
            }
            break;
        }

        case DC_GEOJSON_FEATURE_LINE_STRING: {
            const DcGeojsonCoordArray *coordinates = &feature->geom.line_string;
            plVec3 *points = _planet_geojson_points(planet, coordinates, style.height_above_terrain);
            if (points) {
                dc_app_draw_planet_line(view, points, coordinates->count, style.line_width, line_color, 0);
                PL_FREE(points);
            }
            break;
        }

        case DC_GEOJSON_FEATURE_MULTI_LINE_STRING:
            for (uint32_t i = 0; i < feature->geom.multi_line_string.count; i++) {
                const DcGeojsonCoordArray *coordinates = &feature->geom.multi_line_string.line_strings[i];
                plVec3 *points = _planet_geojson_points(planet, coordinates, style.height_above_terrain);
                if (points) {
                    dc_app_draw_planet_line(view, points, coordinates->count, style.line_width, line_color, 0);
                    PL_FREE(points);
                }
            }
            break;

        case DC_GEOJSON_FEATURE_POLYGON: {
            if (feature->geom.polygon.ring_count == 0) break;
            const DcGeojsonCoordArray *coordinates = &feature->geom.polygon.rings[0];
            plVec3 *points = _planet_geojson_points(planet, coordinates, style.height_above_terrain);
            if (points) {
                if (style.fill_enabled) dc_app_draw_planet_convex_polygon_filled(view, points, coordinates->count, fill_color);
                if (style.line_enabled) dc_app_draw_planet_polygon(view, points, coordinates->count, style.line_width, line_color, 0);
                PL_FREE(points);
            }
            break;
        }

        case DC_GEOJSON_FEATURE_MULTI_POLYGON:
            for (uint32_t i = 0; i < feature->geom.multi_polygon.count; i++) {
                const DcGeojsonPolygon *polygon = &feature->geom.multi_polygon.polygons[i];
                if (polygon->ring_count == 0) continue;
                const DcGeojsonCoordArray *coordinates = &polygon->rings[0];
                plVec3 *points = _planet_geojson_points(planet, coordinates, style.height_above_terrain);
                if (points) {
                    if (style.fill_enabled) dc_app_draw_planet_convex_polygon_filled(view, points, coordinates->count, fill_color);
                    if (style.line_enabled) dc_app_draw_planet_polygon(view, points, coordinates->count, style.line_width, line_color, 0);
                    PL_FREE(points);
                }
            }
            break;

        case DC_GEOJSON_FEATURE_GEOMETRY_COLLECTION:
            for (uint32_t i = 0; i < feature->geom.geometry_collection.count; i++) {
                _planet_draw_geojson_feature(draw_view, &feature->geom.geometry_collection.features[i], style);
            }
            break;

        case DC_GEOJSON_FEATURE_UNDEFINED:
            break;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] draw batch utils
//-----------------------------------------------------------------------------

static dcDrawLayer2D *_draw_batch_get_2d(DcAppDrawContext *ctx) {
    if (!ctx) return NULL;

    int count = sbcount(ctx->sb_draw_batches);
    if (count > 0 && ctx->sb_draw_batches[count - 1].type == DRAW_BATCH_TYPE_2D) {
        _DrawBatch *batch = &ctx->sb_draw_batches[count - 1];
        _ext_dc_draw->set_2d_command_state(batch->draw_list_2d.layer, _command_state(ctx));
        return batch->draw_list_2d.layer;
    }

    // grow pool if needed - request from extension
    int pool_size = sbcount(ctx->sb_draw_list_2d_pool);
    if (ctx->draw_list_2d_index >= pool_size) {
        dcDrawList2D  *new_draw_list = _ext_dc_draw->request_2d_drawlist();
        dcDrawLayer2D *new_layer     = _ext_dc_draw->request_2d_layer(new_draw_list);
        _DrawList2D    new_entry     = {.draw_list = new_draw_list, .layer = new_layer};
        sbpush(ctx->sb_draw_list_2d_pool, new_entry);
    }

    // get draw list + layer from pool
    _DrawList2D *draw_list_2d = &ctx->sb_draw_list_2d_pool[ctx->draw_list_2d_index];
    ctx->draw_list_2d_index++;

    // add batch entry
    _DrawBatch batch = {
        .type         = DRAW_BATCH_TYPE_2D,
        .draw_list_2d = *draw_list_2d};
    sbpush(ctx->sb_draw_batches, batch);

    _ext_dc_draw->set_2d_command_state(draw_list_2d->layer, _command_state(ctx));

    return draw_list_2d->layer;
}

static dcDrawList3D *_draw_batch_get_3d(DcAppDrawContext *ctx) {
    if (!ctx) return NULL;

    int count = sbcount(ctx->sb_draw_batches);
    if (count > 0 && ctx->sb_draw_batches[count - 1].type == DRAW_BATCH_TYPE_3D) {
        _DrawBatch *batch = &ctx->sb_draw_batches[count - 1];
        _ext_dc_draw->set_3d_command_state(batch->draw_list_3d, _command_state(ctx));
        return batch->draw_list_3d;
    }

    // grow pool if needed - request from extension
    int pool_size = sbcount(ctx->sb_draw_list_3d_pool);
    if (ctx->draw_list_3d_index >= pool_size) {
        dcDrawList3D *new_list = _ext_dc_draw->request_3d_drawlist();
        sbpush(ctx->sb_draw_list_3d_pool, new_list);
    }

    // get draw list from pool
    dcDrawList3D *draw_list = ctx->sb_draw_list_3d_pool[ctx->draw_list_3d_index];
    ctx->draw_list_3d_index++;

    // add batch entry
    _DrawBatch batch = {
        .type         = DRAW_BATCH_TYPE_3D,
        .draw_list_3d = draw_list};
    sbpush(ctx->sb_draw_batches, batch);

    _ext_dc_draw->set_3d_command_state(draw_list, _command_state(ctx));

    return draw_list;
}

static void _set_stencil_phase(DcAppDrawContext *ctx, _DcAppStencilPhase phase) {
    int depth = ctx ? sbcount(ctx->stencil.sb_frames) : 0;
    if (depth <= 0 || depth > DC_DRAW_STENCIL_MAX_DEPTH) return;
    if (phase == _DC_APP_STENCIL_PHASE_NONE) return;

    ctx->stencil.phase = phase;
}

static void _restore_stencil_phase(DcAppDrawContext *ctx, _DcAppStencilPhase phase) {
    if (!ctx) return;

    if (sbcount(ctx->stencil.sb_frames) > 0) {
        if (phase == _DC_APP_STENCIL_PHASE_NONE || phase == _DC_APP_STENCIL_PHASE_CLEANUP) {
            phase = _DC_APP_STENCIL_PHASE_DRAW;
        }
        _set_stencil_phase(ctx, phase);
    } else {
        ctx->stencil.phase = _DC_APP_STENCIL_PHASE_NONE;
    }
}

static dcDrawStencilState _stencil_state(_DcAppStencilPhase phase, int depth) {
    if (depth <= 0 || depth > DC_DRAW_STENCIL_MAX_DEPTH) {
        return (dcDrawStencilState){0};
    }

    switch (phase) {
        case _DC_APP_STENCIL_PHASE_CREATE:
            return (dcDrawStencilState){.tMode = DC_DRAW_STENCIL_MODE_CREATE, .uDepth = (uint8_t)depth};
        case _DC_APP_STENCIL_PHASE_REMOVE:
        case _DC_APP_STENCIL_PHASE_CLEANUP:
            return (dcDrawStencilState){.tMode = DC_DRAW_STENCIL_MODE_CLEAR, .uDepth = (uint8_t)depth};
        case _DC_APP_STENCIL_PHASE_DRAW:
            return (dcDrawStencilState){.tMode = DC_DRAW_STENCIL_MODE_DRAW, .uDepth = (uint8_t)depth};
        case _DC_APP_STENCIL_PHASE_NONE:
        default:
            return (dcDrawStencilState){0};
    }
}

static dcDrawCommandState _command_state(DcAppDrawContext *ctx) {
    if (!ctx) return (dcDrawCommandState){0};
    return (dcDrawCommandState){
        .tStencil = _stencil_state(ctx->stencil.phase, sbcount(ctx->stencil.sb_frames)),
    };
}

static void _clear_stencil_bit(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->stencil.sb_frames) == 0) return;

    plIO *io = _ext_ioi ? _ext_ioi->get_io() : NULL;
    if (!io || io->tMainViewportSize.x <= 0.0f || io->tMainViewportSize.y <= 0.0f) return;

    const float w = io->tMainViewportSize.x;
    const float h = io->tMainViewportSize.y;
    plVec2 points[6] = {
        {0.0f, 0.0f},
        {w,    0.0f},
        {w,    h},
        {0.0f, 0.0f},
        {w,    h},
        {0.0f, h},
    };

    _ext_dc_draw->add_triangles_filled(_draw_batch_get_2d(ctx), points, 2, (dcDrawSolidOptions){
        .uColor = PL_COLOR_32_RGBA(0.0f, 0.0f, 0.0f, 1.0f),
    });
}

static _DcAppPlanetContainerFrame *_planet_container_frame(DcAppDrawContext *ctx) {
    if (!ctx || sbcount(ctx->sb_planet_container_stack) == 0) return NULL;
    return &ctx->sb_planet_container_stack[sbcount(ctx->sb_planet_container_stack) - 1];
}

static plVec3 *_planet_container_transform_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count) {
    _DcAppPlanetContainerFrame *frame = _planet_container_frame(ctx);
    if (!frame || !points || point_count == 0) return NULL;

    plVec3 *cartesian = (plVec3 *)PL_ALLOC(sizeof(plVec3) * point_count);
    if (!cartesian) return NULL;

    // Treat local XY as tangent-plane meters and wrap it onto the sphere by arc length.
    for (uint32_t i = 0; i < point_count; i++) {
        double local_x = (double)points[i].x * frame->scale;
        double local_y = (double)points[i].y * frame->scale;
        double x = local_x * frame->rotation_cos - local_y * frame->rotation_sin;
        double y = local_x * frame->rotation_sin + local_y * frame->rotation_cos;
        double distance = hypot(x, y);

        plVec3d direction = frame->up;
        if (distance > 0.0) {
            double angle = distance / frame->planet_radius;
            double tangent_x = (frame->east.x * x + frame->north.x * y) / distance;
            double tangent_y = (frame->east.y * x + frame->north.y * y) / distance;
            double tangent_z = (frame->east.z * x + frame->north.z * y) / distance;
            double angle_cos = cos(angle);
            double angle_sin = sin(angle);
            direction = (plVec3d){
                frame->up.x * angle_cos + tangent_x * angle_sin,
                frame->up.y * angle_cos + tangent_y * angle_sin,
                frame->up.z * angle_cos + tangent_z * angle_sin,
            };
        }

        cartesian[i] = (plVec3){
            (float)(direction.x * frame->surface_radius),
            (float)(direction.y * frame->surface_radius),
            (float)(direction.z * frame->surface_radius),
        };
    }
    return cartesian;
}

static void _apply_planet_view_options(DcAppDrawPlanetViewHandle draw_view) {
    if (!draw_view) return;

    plPlanetView *view = dc_app_planet_view_pl(draw_view->view);
    if (!view) return;

    plPlanetViewRuntimeOptions options = _ext_planet->get_view_runtime_options(view);
    options.tFlags = draw_view->options.flags;
    options.fTau = draw_view->options.tau > 0.0f ? draw_view->options.tau : 0.3f;
    _ext_planet->set_view_runtime_options(view, options);
}

static void _flush_planet_views(DcAppDrawContext *ctx, int first_view) {
    if (!ctx) return;
    int view_count = sbcount(ctx->sb_planet_views);
    if (first_view < 0) first_view = 0;
    if (first_view > view_count) first_view = view_count;

    for (int i = first_view; i < view_count; i++) {
        DcAppDrawPlanetViewHandle draw_view = ctx->sb_planet_views[i];
        if (!draw_view) continue;

        plPlanetView *view = dc_app_planet_view_pl(draw_view->view);
        if (!view) continue;

        _apply_planet_view_options(draw_view);

        // renders the queued planet view into the texture drawn at call time.
        plCommandBuffer *cmd_buf = _ext_starter->get_command_buffer();
        _ext_planet->render_view(view, &draw_view->camera, cmd_buf, draw_view->logical_dimensions);
        _ext_starter->submit_command_buffer(cmd_buf);
    }

    for (int i = first_view; i < view_count; i++) {
        if (ctx->sb_planet_views[i]) PL_FREE(ctx->sb_planet_views[i]);
    }
    sbpopn(ctx->sb_planet_views, view_count - first_view);
}

static plCamera _planet_camera_base(float fov_degrees, bool orthographic, DcAppVec2 size) {
    plCamera camera = {0};
    camera.tType        = PL_CAMERA_TYPE_PERSPECTIVE_REVERSE_Z;
    camera.fFieldOfView = (fov_degrees > 0.0f && fov_degrees < 180.0f) ? pl_radiansf(fov_degrees) : PL_PI_3;
    camera.fAspectRatio = (size.y > 0.0f) ? size.x / size.y : 1.0f;
    camera.fNearZ       = 1.0f;
    camera.fFarZ        = 100000000.0f;
    camera.fWidth       = size.x;
    camera.fHeight      = size.y;
    if (orthographic) camera.tType = PL_CAMERA_TYPE_ORTHOGRAPHIC_REVERSE_Z;
    return camera;
}

static void _planet_camera_apply_distance_ortho(DcAppPlanetHandle planet, plCamera *camera) {
    if (!planet || !camera) return;

    // derives orthographic scale from camera altitude to match xml and snapshot behavior.
    double cam_dist = sqrt(camera->tPosDouble.x * camera->tPosDouble.x +
                           camera->tPosDouble.y * camera->tPosDouble.y +
                           camera->tPosDouble.z * camera->tPosDouble.z);
    double surface_dist = cam_dist - dc_app_planet_radius(planet);
    if (surface_dist < 1.0) surface_dist = 1.0;
    float half_h = (float)surface_dist * tanf(camera->fFieldOfView / 2.0f);
    float half_w = half_h * camera->fAspectRatio;
    camera->tProjMat = (plMat4){0};
    camera->tProjMat.col[0].x = 1.0f / half_w;
    camera->tProjMat.col[1].y = 1.0f / half_h;
    camera->tProjMat.col[2].z = 1.0f / (camera->fFarZ - camera->fNearZ);
    camera->tProjMat.col[3].w = 1.0f;
}

static bool _planet_project_overlay(DcAppDrawPlanetViewHandle draw_view, DcAppVec3d position, float size_meters, DcAppVec2 *out_position, float *out_size) {
    if (out_position) *out_position = (DcAppVec2){0};
    if (out_size) *out_size = 0.0f;
    if (!draw_view || !draw_view->view || !out_position || !out_size || size_meters <= 0.0f) return false;

    uint32_t output_width_px = dc_app_planet_view_width(draw_view->view);
    uint32_t output_height_px = dc_app_planet_view_height(draw_view->view);
    if (output_width_px == 0 || output_height_px == 0) return false;

    DcAppPlanetHandle planet = dc_app_planet_view_planet(draw_view->view);
    double planet_radius = dc_app_planet_radius(planet);
    double dx = position.x - draw_view->camera.tPosDouble.x;
    double dy = position.y - draw_view->camera.tPosDouble.y;
    double dz = position.z - draw_view->camera.tPosDouble.z;
    if (planet_radius > 0.0) {
        double a = dx * dx + dy * dy + dz * dz;
        double b = 2.0 * (draw_view->camera.tPosDouble.x * dx +
                          draw_view->camera.tPosDouble.y * dy +
                          draw_view->camera.tPosDouble.z * dz);
        double c = draw_view->camera.tPosDouble.x * draw_view->camera.tPosDouble.x +
                   draw_view->camera.tPosDouble.y * draw_view->camera.tPosDouble.y +
                   draw_view->camera.tPosDouble.z * draw_view->camera.tPosDouble.z -
                   planet_radius * planet_radius;
        double discriminant = b * b - 4.0 * a * c;
        if (a > 0.000001 && discriminant > 0.0) {
            double t = (-b - sqrt(discriminant)) / (2.0 * a);
            if (t > 0.0 && t < 0.999999) return false;
        }
    }

    plMat4 mvp = pl_mul_mat4(&draw_view->camera.tProjMat, &draw_view->camera.tViewMatDouble);
    plVec4 projected = pl_mul_mat4_vec4(
        &mvp,
        (plVec4){(float)dx, (float)dy, (float)dz, 1.0f});
    if (fabsf(projected.w) <= 0.000001f) return false;
    projected = pl_div_vec4_scalarf(projected, projected.w);
    if (projected.z < 0.0f || projected.z > 1.0f) return false;

    float output_width = (float)output_width_px;
    float output_height = (float)output_height_px;
    float pixel_x = output_width * 0.5f * (1.0f + projected.x);
    float pixel_y = output_height * 0.5f * (1.0f + projected.y);
    if (pixel_x < 0.0f || pixel_x > output_width || pixel_y < 0.0f || pixel_y > output_height) return false;

    double distance = sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 0.001) distance = 0.001;

    float pixel_size = (float)((double)size_meters * (double)output_height /
                               (2.0 * distance * tan((double)draw_view->camera.fFieldOfView * 0.5)));
    if (pixel_size < 1.0f) pixel_size = 1.0f;
    if (pixel_size > 500.0f) pixel_size = 500.0f;

    float scale_x = draw_view->area.dimensions[0] / output_width;
    float scale_y = draw_view->area.dimensions[1] / output_height;
    *out_position = (DcAppVec2){pixel_x * scale_x, (output_height - pixel_y) * scale_y};
    *out_size = pixel_size * scale_y;
    return *out_size > 0.0f;
}

static DcAppVec2 _planet_image_size_meters(DcAppDrawContext *ctx, DcAppTextureId texture_id, DcAppVec2 size) {
    if (size.x > 0.0f && size.y > 0.0f) return size;
    if (!ctx || texture_id == 0) return (DcAppVec2){0};

    DcAppVec2 texture_size = {0};
    if (!dc_app_texture_get_size(ctx->texture_ctx, texture_id, &texture_size)) return (DcAppVec2){0};
    if (texture_size.x <= 0.0f || texture_size.y <= 0.0f) return (DcAppVec2){0};

    float aspect = texture_size.x / texture_size.y;
    if (size.x > 0.0f) return (DcAppVec2){size.x, size.x / aspect};
    if (size.y > 0.0f) return (DcAppVec2){size.y * aspect, size.y};
    return (DcAppVec2){0};
}

static void _planet_draw_image_label(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint) {
    if (!ctx || !draw_view || texture_id == 0 || size.x <= 0.0f || size.y <= 0.0f) return;
    if (draw_view->area.dimensions[0] <= 0.0f || draw_view->area.dimensions[1] <= 0.0f) return;
    if (!dc_app_draw_container_push_area(ctx, &draw_view->area)) return;

    DcAppPlacement centered = {
        .local_align_x = DC_APP_ALIGN_TYPE_CENTER,
        .local_align_y = DC_APP_ALIGN_TYPE_MIDDLE,
    };

    if (!dc_app_draw_stencil_begin(ctx)) {
        dc_app_draw_image_ex(ctx, texture_id, position, size, tint, centered, NULL);
        dc_app_draw_container_pop(ctx);
        return;
    }

    dc_app_draw_stencil_add(ctx);
    dc_app_draw_quad_filled_ex(ctx,
                               (DcAppVec2){0.0f, 0.0f},
                               (DcAppVec2){draw_view->area.dimensions[0], 0.0f},
                               (DcAppVec2){draw_view->area.dimensions[0], draw_view->area.dimensions[1]},
                               (DcAppVec2){0.0f, draw_view->area.dimensions[1]},
                               (DcAppVec4){1.0f, 1.0f, 1.0f, 1.0f},
                               (DcAppVec2){0.0f, 0.0f},
                               (DcAppPlacement){0},
                               NULL);

    dc_app_draw_stencil_draw(ctx);
    dc_app_draw_image_ex(ctx, texture_id, position, size, tint, centered, NULL);
    dc_app_draw_stencil_end(ctx);
    dc_app_draw_container_pop(ctx);
}

static void _planet_draw_text_label(DcAppDrawContext *ctx, DcAppDrawPlanetViewHandle draw_view, DcAppVec2 position, const char *text, float size, DcAppVec4 color) {
    if (!ctx || !draw_view || !text || size <= 0.0f) return;
    if (draw_view->area.dimensions[0] <= 0.0f || draw_view->area.dimensions[1] <= 0.0f) return;
    if (!dc_app_draw_container_push_area(ctx, &draw_view->area)) return;

    DcAppPlacement centered = {
        .local_align_x = DC_APP_ALIGN_TYPE_CENTER,
        .local_align_y = DC_APP_ALIGN_TYPE_MIDDLE,
    };

    if (!dc_app_draw_stencil_begin(ctx)) {
        dc_app_draw_text_ex(ctx, position, text, (DcAppTextStyle){.size = size, .color = color}, centered, NULL);
        dc_app_draw_container_pop(ctx);
        return;
    }

    dc_app_draw_stencil_add(ctx);
    dc_app_draw_quad_filled_ex(ctx,
                               (DcAppVec2){0.0f, 0.0f},
                               (DcAppVec2){draw_view->area.dimensions[0], 0.0f},
                               (DcAppVec2){draw_view->area.dimensions[0], draw_view->area.dimensions[1]},
                               (DcAppVec2){0.0f, draw_view->area.dimensions[1]},
                               (DcAppVec4){1.0f, 1.0f, 1.0f, 1.0f},
                               (DcAppVec2){0.0f, 0.0f},
                               (DcAppPlacement){0},
                               NULL);

    dc_app_draw_stencil_draw(ctx);
    dc_app_draw_text_ex(ctx, position, text, (DcAppTextStyle){.size = size, .color = color}, centered, NULL);
    dc_app_draw_stencil_end(ctx);
    dc_app_draw_container_pop(ctx);
}

static plCamera _planet_camera_geodetic(DcAppPlanetHandle planet, double lat, double lon, double elevation, DcAppVec3 rpy_degrees, float fov_degrees, bool orthographic, DcAppVec2 size) {
    plCamera camera = _planet_camera_base(fov_degrees, orthographic, size);
    if (!planet) return camera;

    const DcGeoCrsGeodetic *geodetic_crs = dc_app_planet_geodetic_crs(planet);
    const DcGeoCrsCartesian *cartesian_crs = dc_app_planet_cartesian_crs(planet);
    if (!geodetic_crs || !cartesian_crs) return camera;

    double lat_rad = dc_utils_degrees_to_radians(lat);
    double lon_rad = dc_utils_degrees_to_radians(lon);
    plVec3d geodetic_in = {lat, lon, elevation};
    plVec3d eye;
    dc_geo_geodetic_to_cartesian_d(geodetic_crs, cartesian_crs, &geodetic_in, &eye, 1);

    plVec3 north, east, down, up;
    dc_geo_get_local_ned_basis(lat_rad, lon_rad, &north, &east, &down, &up);
    // applies local-ned attitude as yaw about down, pitch about right, and roll about boresight.
    float yaw = pl_radiansf(rpy_degrees.yaw);
    float pitch = pl_radiansf(rpy_degrees.pitch);
    float roll = pl_radiansf(rpy_degrees.roll);
    plVec3 forward = down;
    plVec3 right = dc_geo_rotate_vector_around_axis(east, down, yaw);
    plVec3 desired_up = dc_geo_rotate_vector_around_axis(north, down, yaw);
    forward = dc_geo_rotate_vector_around_axis(forward, right, pitch);
    desired_up = dc_geo_rotate_vector_around_axis(desired_up, right, pitch);
    desired_up = dc_geo_rotate_vector_around_axis(desired_up, forward, roll);

    plVec3d target = {
        eye.x + (double)forward.x,
        eye.y + (double)forward.y,
        eye.z + (double)forward.z
    };
    _ext_camera->look_at(&camera, eye, target);
    camera.fRoll = 0.0f;
    _ext_camera->update(&camera);
    camera.fRoll = dc_geo_signed_angle_around_axis(camera._tUpVec, desired_up, forward);
    _ext_camera->update(&camera);

    if (orthographic) _planet_camera_apply_distance_ortho(planet, &camera);

    return camera;
}

static plCamera _planet_camera_cartesian(DcAppPlanetHandle planet, DcAppVec3d position, DcAppVec3 rpy_degrees, float fov_degrees, bool orthographic, DcAppVec2 size) {
    plCamera camera = _planet_camera_base(fov_degrees, orthographic, size);
    // applies cartesian attitude through pilotlight camera pitch, yaw, and roll.
    _ext_camera->set_pos(&camera, position.x, position.y, position.z);
    _ext_camera->set_pitch_yaw(&camera, pl_radiansf(rpy_degrees.pitch), pl_radiansf(rpy_degrees.yaw));
    camera.fRoll = pl_radiansf(rpy_degrees.roll);
    _ext_camera->update(&camera);
    if (orthographic) _planet_camera_apply_distance_ortho(planet, &camera);
    return camera;
}
