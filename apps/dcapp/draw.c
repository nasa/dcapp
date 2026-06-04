#define _USE_MATH_DEFINES
#include <math.h>

#include "dcapp.h"
#include "draw.h"

#include "app/enums.h"

static const DcAppDrawApi dc_app_draw_interface = {
    .get_area              = dc_app_draw_get_area,
    .line                   = dc_app_draw_line,
    .line_ex                = dc_app_draw_line_ex,
    .polyline               = dc_app_draw_polyline,
    .polyline_ex            = dc_app_draw_polyline_ex,
    .polygon                = dc_app_draw_polygon,
    .polygon_ex             = dc_app_draw_polygon_ex,
    .polygon_filled         = dc_app_draw_polygon_filled,
    .polygon_filled_ex      = dc_app_draw_polygon_filled_ex,
    .rounded_polygon        = dc_app_draw_rounded_polygon,
    .rounded_polygon_ex     = dc_app_draw_rounded_polygon_ex,
    .rounded_polygon_filled    = dc_app_draw_rounded_polygon_filled,
    .rounded_polygon_filled_ex = dc_app_draw_rounded_polygon_filled_ex,
    .quad                   = dc_app_draw_quad,
    .quad_ex                = dc_app_draw_quad_ex,
    .quad_filled            = dc_app_draw_quad_filled,
    .quad_filled_ex         = dc_app_draw_quad_filled_ex,
    .rounded_quad           = dc_app_draw_rounded_quad,
    .rounded_quad_ex        = dc_app_draw_rounded_quad_ex,
    .rounded_quad_filled    = dc_app_draw_rounded_quad_filled,
    .rounded_quad_filled_ex = dc_app_draw_rounded_quad_filled_ex,
    .image                  = dc_app_draw_image,
    .image_ex               = dc_app_draw_image_ex,
    .rect                   = dc_app_draw_rect,
    .rect_ex                = dc_app_draw_rect_ex,
    .rect_filled            = dc_app_draw_rect_filled,
    .rect_filled_ex         = dc_app_draw_rect_filled_ex,
    .rounded_rect           = dc_app_draw_rounded_rect,
    .rounded_rect_ex        = dc_app_draw_rounded_rect_ex,
    .rounded_rect_filled    = dc_app_draw_rounded_rect_filled,
    .rounded_rect_filled_ex = dc_app_draw_rounded_rect_filled_ex,
    .circle                 = dc_app_draw_circle,
    .circle_ex              = dc_app_draw_circle_ex,
    .circle_filled          = dc_app_draw_circle_filled,
    .circle_filled_ex       = dc_app_draw_circle_filled_ex,
    .ellipse                = dc_app_draw_ellipse,
    .ellipse_ex             = dc_app_draw_ellipse_ex,
    .ellipse_filled         = dc_app_draw_ellipse_filled,
    .ellipse_filled_ex      = dc_app_draw_ellipse_filled_ex,
    .text_size              = dc_app_draw_text_size,
    .text                   = dc_app_draw_text,
    .text_ex                = dc_app_draw_text_ex,
    .container_push         = dc_app_draw_container_push,
    .container_push_ex      = dc_app_draw_container_push_ex,
    .container_push_area    = dc_app_draw_container_push_area,
    .container_pop          = dc_app_draw_container_pop,
    .stencil_begin          = dc_app_draw_stencil_begin,
    .stencil_add            = dc_app_draw_stencil_add,
    .stencil_remove         = dc_app_draw_stencil_remove,
    .stencil_draw           = dc_app_draw_stencil_draw,
    .stencil_end            = dc_app_draw_stencil_end,
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

typedef enum _DcAppStencilPhase {
    _DC_APP_STENCIL_PHASE_NONE,
    _DC_APP_STENCIL_PHASE_CREATE,
    _DC_APP_STENCIL_PHASE_REMOVE,
    _DC_APP_STENCIL_PHASE_DRAW,
    _DC_APP_STENCIL_PHASE_CLEANUP,
} _DcAppStencilPhase;

typedef enum _DcAppDrawCommandType {
    _DC_APP_DRAW_COMMAND_LINES,
    _DC_APP_DRAW_COMMAND_TRIANGLES_FILLED,
    _DC_APP_DRAW_COMMAND_POLYGON,
    _DC_APP_DRAW_COMMAND_POLYGON_FILLED,
    _DC_APP_DRAW_COMMAND_IMAGE_QUAD,
    _DC_APP_DRAW_COMMAND_IMAGE_QUAD_UV,
    _DC_APP_DRAW_COMMAND_TEXT,
} _DcAppDrawCommandType;

typedef struct _DcAppDrawCommand {
    _DcAppDrawCommandType type;
    plVec2 *sb_points;
    uint32_t point_count;
    float corner_radius;
    DcAppStroke stroke;
    DcAppVec4 color;
    uint32_t texture_id;
    DcAppVec2 uv0;
    DcAppVec2 uv1;
    DcAppVec2 uv2;
    DcAppVec2 uv3;
    DcAppVec4 tint;
    char *text;
    dcDrawTextOptions text_options;
} _DcAppDrawCommand;

typedef struct _DcAppStencilFrame {
    _DcAppDrawCommand *sb_add_commands;
    _DcAppStencilPhase previous_phase;
    DcAppStencilHandler handler;
} _DcAppStencilFrame;

typedef struct _DcAppStencilRecorder {
    _DcAppStencilFrame *sb_frames;
    _DcAppStencilPhase phase;
    bool is_replaying;
} _DcAppStencilRecorder;

typedef struct _DcAppContainerData {
    uint32_t count;
    DcAppDrawArea stack[DCAPP_DRAW_CONTEXT_STACK_MAX];
} _DcAppContainerData;

static void _set_stencil_phase(_AppData *app_data, int depth, _DcAppStencilPhase phase);
static void _restore_stencil_phase(_AppData *app_data, _DcAppStencilPhase phase);
static bool _placement_is_default(DcAppPlacement placement);
static DcAppDrawArea *_draw_result_area(DcAppDrawResult *result);
static void _draw_context_update_mouse(_AppData *app_data, DcAppDrawContext *ctx);
static void _draw_area_from_rect_points(float width, float height, plVec2 p0, plVec2 p1, plVec2 p3, DcAppDrawArea *out_area);
static void _resolve_rect_points(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 out[4], DcAppDrawArea *out_area);
static plVec2 *_alloc_resolved_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area);
static uint64_t _mouse_id(const char *id);
static bool _mouse_rect_local(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 *out);
static bool _mouse_point_in_polygon(plVec2 mouse, const plVec2 *points, uint32_t point_count);
static void _mouse_register(DcAppDrawContext *ctx, uint64_t id);
static bool _resolve_texture_id(DcAppDrawContext *ctx, DcAppTextureId texture_id, uint32_t *out);
static void _draw_image_uv(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 dimensions, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area);
static _DcAppContainerData *_container_data(DcAppDrawContext *ctx);
static _DcAppStencilRecorder *_stencil_recorder(DcAppDrawContext *ctx);
static dcDrawTextOptions _text_options(_AppData *app_data, DcAppTextStyle style);
static plMat3 _text_transform(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement);
static void _record_stencil_add_command(DcAppDrawContext *ctx, const _DcAppDrawCommand *command);
static void _replay_stencil_command(_AppData *app_data, const _DcAppDrawCommand *command);
static void _free_stencil_command(_DcAppDrawCommand *command);

//-----------------------------------------------------------------------------
// [SECTION] DrawFunction context helpers
//-----------------------------------------------------------------------------

DcAppDrawContext dc_app_draw_context(_AppData *app_data, _NodeIndex node_index, plVec2 parent_position, plVec2 parent_dimensions, const plMat4 *parent_transform) {
    (void)node_index;

    plMat4 transform = parent_transform ? *parent_transform : pl_identity_mat4();
    DcAppDrawContext ctx = {
        ._runtime = app_data,
    };

    ctx.area.position[0]   = parent_position.x;
    ctx.area.position[1]   = parent_position.y;
    ctx.area.dimensions[0] = parent_dimensions.x;
    ctx.area.dimensions[1] = parent_dimensions.y;
    memcpy(ctx.area.transform, transform.d, sizeof(ctx.area.transform));
    _draw_context_update_mouse(app_data, &ctx);

    return ctx;
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

static void _draw_context_update_mouse(_AppData *app_data, DcAppDrawContext *ctx) {
    if (!ctx) return;

    ctx->mouse = (DcAppMouse){0};
    if (!app_data) return;

    plMat4 transform = pl_identity_mat4();
    memcpy(transform.d, ctx->area.transform, sizeof(transform.d));

    plMat4 inv_transform = pl_mat4t_invert(&transform);
    plVec4 mouse_screen  = {
        app_data->frame_data.mouse_position.x,
        app_data->frame_data.mouse_position.y,
        0.0f,
        1.0f,
    };
    plVec4 mouse_local = pl_mul_mat4_vec4(&inv_transform, mouse_screen);

    ctx->mouse.x              = mouse_local.x;
    ctx->mouse.y              = mouse_local.y;
    ctx->mouse.position_valid = app_data->frame_data.is_mouse_position_valid;
    ctx->mouse.pressed        = app_data->frame_data.is_mouse_pressed;
    ctx->mouse.released       = app_data->frame_data.is_mouse_released;
    ctx->mouse.down           = app_data->frame_data.is_mouse_down;
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

static bool _mouse_rect_local(DcAppDrawContext *ctx, DcAppVec2 dimensions, DcAppVec2 position, DcAppPlacement placement, plVec2 *out) {
    if (!ctx || !ctx->_runtime || !out || dimensions.x == 0.0f || dimensions.y == 0.0f) return false;

    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data->frame_data.is_mouse_position_valid) return false;

    plVec2 points[4];
    _resolve_rect_points(ctx, dimensions, position, placement, points, NULL);

    plVec2 p0 = points[0];
    plVec2 p1 = points[1];
    plVec2 p3 = points[3];
    plVec2 vx = {p1.x - p0.x, p1.y - p0.y};
    plVec2 vy = {p3.x - p0.x, p3.y - p0.y};
    plVec2 pm = {app_data->frame_data.mouse_position.x - p0.x, app_data->frame_data.mouse_position.y - p0.y};

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
    if (!ctx || !ctx->_runtime || id == 0) return;

    _AppData *app_data = (_AppData *)ctx->_runtime;
    app_data->frame_data.next_hovered_target = _mouse_target_id(id);
    if (app_data->frame_data.is_mouse_pressed) {
        app_data->frame_data.next_pressed_target = _mouse_target_id(id);
    }
}

static dcDrawTextOptions _text_options(_AppData *app_data, DcAppTextStyle style) {
    dcDrawTextOptions options = {0};

    options.ptFont = app_data ? app_data->pl_vera_sdf_font : NULL;
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
    if (!ctx || size.x == 0.0f || size.y == 0.0f) return false;
    DcAppDrawArea *out_area = _draw_result_area(result);

    if (virtual_size.x == 0.0f) {
        virtual_size.x = size.x;
    }
    if (virtual_size.y == 0.0f) {
        virtual_size.y = size.y;
    }
    if (virtual_size.x == 0.0f || virtual_size.y == 0.0f) return false;

    _DcAppContainerData *data = _container_data(ctx);
    if (!data || data->count >= DCAPP_DRAW_CONTEXT_STACK_MAX) return false;

    DcAppDrawArea *state = &data->stack[data->count++];
    *state = ctx->area;

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

    _draw_context_update_mouse((_AppData *)ctx->_runtime, ctx);
    if (out_area) *out_area = ctx->area;

    return true;
}

bool dc_app_draw_container_push_area(DcAppDrawContext *ctx, const DcAppDrawArea *area) {
    if (!ctx || !area) return false;

    _DcAppContainerData *data = _container_data(ctx);
    if (!data || data->count >= DCAPP_DRAW_CONTEXT_STACK_MAX) return false;

    DcAppDrawArea *state = &data->stack[data->count++];
    *state = ctx->area;

    ctx->area = *area;
    _draw_context_update_mouse((_AppData *)ctx->_runtime, ctx);
    return true;
}

void dc_app_draw_container_pop(DcAppDrawContext *ctx) {
    if (!ctx || !ctx->_container_data) return;

    _DcAppContainerData *data = (_DcAppContainerData *)ctx->_container_data;
    if (data->count == 0) return;

    DcAppDrawArea *state = &data->stack[--data->count];
    ctx->area = *state;
    _draw_context_update_mouse((_AppData *)ctx->_runtime, ctx);

    if (data->count == 0) {
        PL_FREE(data);
        ctx->_container_data = NULL;
    }
}

bool dc_app_draw_stencil_begin_handler(_AppData *app_data, DcAppStencilHandler *handler) {
    if (!app_data || !handler || app_data->stencil_depth >= DC_STENCIL_MAX_DEPTH) return false;

    app_data->stencil_phase_stack[app_data->stencil_depth] = app_data->stencil_phase;
    *handler = (DcAppStencilHandler){
        .depth = ++app_data->stencil_depth,
    };
    return true;
}

void dc_app_draw_set_stencil_add(_AppData *app_data, const DcAppStencilHandler *handler) {
    if (!app_data || !handler) return;
    _set_stencil_phase(app_data, handler->depth, _DC_APP_STENCIL_PHASE_CREATE);
}

void dc_app_draw_set_stencil_remove(_AppData *app_data, const DcAppStencilHandler *handler) {
    if (!app_data || !handler) return;
    _set_stencil_phase(app_data, handler->depth, _DC_APP_STENCIL_PHASE_REMOVE);
}

void dc_app_draw_set_stencil_draw(_AppData *app_data, const DcAppStencilHandler *handler) {
    if (!app_data || !handler) return;
    _set_stencil_phase(app_data, handler->depth, _DC_APP_STENCIL_PHASE_DRAW);
}

void dc_app_draw_set_stencil_cleanup(_AppData *app_data, const DcAppStencilHandler *handler) {
    if (!app_data || !handler) return;
    _set_stencil_phase(app_data, handler->depth, _DC_APP_STENCIL_PHASE_CLEANUP);
}

void dc_app_draw_stencil_end_handler(_AppData *app_data, const DcAppStencilHandler *handler) {
    if (!app_data || !handler) return;
    if (handler->depth != app_data->stencil_depth) return;

    _DcAppStencilPhase previous_phase = (_DcAppStencilPhase)app_data->stencil_phase_stack[handler->depth - 1];
    app_data->stencil_phase_stack[handler->depth - 1] = _DC_APP_STENCIL_PHASE_NONE;
    app_data->stencil_depth--;
    _restore_stencil_phase(app_data, previous_phase);
}

bool dc_app_draw_stencil_begin(DcAppDrawContext *ctx) {
    if (!ctx) return false;

    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return false;

    _DcAppStencilRecorder *recorder = _stencil_recorder(ctx);
    if (!recorder) return false;

    DcAppStencilHandler handler;
    if (!dc_app_draw_stencil_begin_handler(app_data, &handler)) return false;

    _DcAppStencilFrame frame = {
        .previous_phase = recorder->phase,
        .handler        = handler,
    };
    sbpush(recorder->sb_frames, frame);

    recorder->phase = _DC_APP_STENCIL_PHASE_CREATE;
    dc_app_draw_set_stencil_add(app_data, &handler);
    return true;
}

void dc_app_draw_stencil_add(DcAppDrawContext *ctx) {
    if (!ctx) return;
    _AppData *app_data = (_AppData *)ctx->_runtime;
    _DcAppStencilRecorder *recorder = _stencil_recorder(ctx);
    if (!app_data || !recorder || app_data->stencil_depth <= 0 || sbcount(recorder->sb_frames) <= 0) return;
    _DcAppStencilFrame *frame = &recorder->sb_frames[sbcount(recorder->sb_frames) - 1];

    recorder->phase = _DC_APP_STENCIL_PHASE_CREATE;
    dc_app_draw_set_stencil_add(app_data, &frame->handler);
}

void dc_app_draw_stencil_remove(DcAppDrawContext *ctx) {
    if (!ctx) return;
    _AppData *app_data = (_AppData *)ctx->_runtime;
    _DcAppStencilRecorder *recorder = _stencil_recorder(ctx);
    if (!app_data || !recorder || app_data->stencil_depth <= 0 || sbcount(recorder->sb_frames) <= 0) return;
    _DcAppStencilFrame *frame = &recorder->sb_frames[sbcount(recorder->sb_frames) - 1];

    recorder->phase = _DC_APP_STENCIL_PHASE_REMOVE;
    dc_app_draw_set_stencil_remove(app_data, &frame->handler);
}

void dc_app_draw_stencil_draw(DcAppDrawContext *ctx) {
    if (!ctx) return;
    _AppData *app_data = (_AppData *)ctx->_runtime;
    _DcAppStencilRecorder *recorder = _stencil_recorder(ctx);
    if (!app_data || !recorder || app_data->stencil_depth <= 0 || sbcount(recorder->sb_frames) <= 0) return;
    _DcAppStencilFrame *frame = &recorder->sb_frames[sbcount(recorder->sb_frames) - 1];

    recorder->phase = _DC_APP_STENCIL_PHASE_DRAW;
    dc_app_draw_set_stencil_draw(app_data, &frame->handler);
}

void dc_app_draw_stencil_end(DcAppDrawContext *ctx) {
    if (!ctx) return;

    _AppData *app_data = (_AppData *)ctx->_runtime;
    _DcAppStencilRecorder *recorder = (_DcAppStencilRecorder *)ctx->_stencil_data;
    if (!app_data || !recorder || app_data->stencil_depth <= 0 || sbcount(recorder->sb_frames) <= 0) return;

    _DcAppStencilFrame *frame = &recorder->sb_frames[sbcount(recorder->sb_frames) - 1];
    dc_app_draw_set_stencil_cleanup(app_data, &frame->handler);

    recorder->is_replaying = true;
    for (int ii = 0; ii < sbcount(frame->sb_add_commands); ii++) {
        _replay_stencil_command(app_data, &frame->sb_add_commands[ii]);
    }
    recorder->is_replaying = false;

    for (int ii = 0; ii < sbcount(frame->sb_add_commands); ii++) {
        _free_stencil_command(&frame->sb_add_commands[ii]);
    }
    sbfree(frame->sb_add_commands);

    _DcAppStencilPhase previous_phase = frame->previous_phase;
    DcAppStencilHandler handler = frame->handler;
    sbpop(recorder->sb_frames);
    recorder->phase = previous_phase;

    dc_app_draw_stencil_end_handler(app_data, &handler);

    if (sbcount(recorder->sb_frames) == 0) {
        sbfree(recorder->sb_frames);
        PL_FREE(recorder);
        ctx->_stencil_data = NULL;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DrawFunction mouse API
//-----------------------------------------------------------------------------

const DcAppMouse *dc_app_mouse_get_state(DcAppDrawContext *ctx) {
    return ctx ? &ctx->mouse : NULL;
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
    if (!ctx || !ctx->_runtime || mouse_id == 0 || !points || point_count < 3) return;

    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data->frame_data.is_mouse_position_valid) return;

    plVec2 *resolved = _alloc_resolved_points(ctx, points, point_count, position, placement, NULL);
    if (!resolved) return;

    bool inside = _mouse_point_in_polygon(app_data->frame_data.mouse_position, resolved, point_count);
    PL_FREE(resolved);

    if (inside) {
        _mouse_register(ctx, mouse_id);
    }
}

bool dc_app_mouse_hovered(DcAppDrawContext *ctx, const char *id) {
    if (!ctx || !ctx->_runtime) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _mouse_target_is_id(((_AppData *)ctx->_runtime)->frame_data.hovered_target, mouse_id);
}

bool dc_app_mouse_pressed(DcAppDrawContext *ctx, const char *id) {
    if (!ctx || !ctx->_runtime) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _mouse_target_is_id(((_AppData *)ctx->_runtime)->frame_data.pressed_target, mouse_id);
}

bool dc_app_mouse_released(DcAppDrawContext *ctx, const char *id) {
    if (!ctx || !ctx->_runtime) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _mouse_target_is_id(((_AppData *)ctx->_runtime)->frame_data.released_target, mouse_id);
}

bool dc_app_mouse_active(DcAppDrawContext *ctx, const char *id) {
    if (!ctx || !ctx->_runtime) return false;
    uint64_t mouse_id = _mouse_id(id);
    return mouse_id != 0 && _mouse_target_is_id(((_AppData *)ctx->_runtime)->frame_data.active_target, mouse_id);
}

bool dc_app_mouse_clicked(DcAppDrawContext *ctx, const char *id) {
    return dc_app_mouse_released(ctx, id);
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

void dc_app_draw_polygon_filled(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color) {
    dc_app_draw_polygon_filled_ex(ctx, points, point_count, color, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_polygon(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke) {
    dc_app_draw_rounded_polygon_ex(ctx, points, point_count, corner_radius, stroke, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
}

void dc_app_draw_rounded_polygon_filled(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color) {
    dc_app_draw_rounded_polygon_filled_ex(ctx, points, point_count, corner_radius, color, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
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
    if (!ctx || !points || point_count < 2) return;
    DcAppDrawArea *out_area = _draw_result_area(result);
    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type        = _DC_APP_DRAW_COMMAND_LINES,
        .sb_points   = draw_points,
        .point_count = point_count,
        .stroke      = stroke,
    });

    _ext_dc_draw->add_lines(dc_app_draw_batch_get_2d(app_data), draw_points, point_count, (dcDrawLineOptions){
        .uColor       = PL_COLOR_32_RGBA(stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a),
        .fThickness   = stroke.width * DCAPP_LINE_WIDTH_FACTOR,
        .uDashPattern = stroke.pattern,
    });
    PL_FREE(draw_points);
}

void dc_app_draw_triangles_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (!ctx || !points || point_count < 3) return;
    DcAppDrawArea *out_area = _draw_result_area(result);
    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type        = _DC_APP_DRAW_COMMAND_TRIANGLES_FILLED,
        .sb_points   = draw_points,
        .point_count = point_count,
        .color       = color,
    });

    _ext_dc_draw->add_triangles_filled(dc_app_draw_batch_get_2d(app_data), draw_points, point_count, (dcDrawSolidOptions){
        .uColor = PL_COLOR_32_RGBA(color.r, color.g, color.b, color.a),
    });
    PL_FREE(draw_points);
}

void dc_app_draw_polygon_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_polygon_ex(ctx, points, point_count, 0.0f, stroke, position, placement, result);
}

void dc_app_draw_polygon_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    dc_app_draw_rounded_polygon_filled_ex(ctx, points, point_count, 0.0f, color, position, placement, result);
}

void dc_app_draw_rounded_polygon_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (!ctx || !points || point_count < 3) return;
    DcAppDrawArea *out_area = _draw_result_area(result);
    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type          = _DC_APP_DRAW_COMMAND_POLYGON,
        .sb_points     = draw_points,
        .point_count   = point_count,
        .corner_radius = corner_radius,
        .stroke        = stroke,
    });

    dcDrawLayer2D *layer = dc_app_draw_batch_get_2d(app_data);
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

void dc_app_draw_rounded_polygon_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result) {
    if (!ctx || !points || point_count < 3) return;
    DcAppDrawArea *out_area = _draw_result_area(result);
    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    plVec2 *draw_points = _alloc_resolved_points(ctx, points, point_count, position, placement, out_area);
    if (!draw_points) return;
    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type          = _DC_APP_DRAW_COMMAND_POLYGON_FILLED,
        .sb_points     = draw_points,
        .point_count   = point_count,
        .corner_radius = corner_radius,
        .color         = color,
    });

    dcDrawLayer2D *layer = dc_app_draw_batch_get_2d(app_data);
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
    dc_app_draw_rounded_polygon_filled_ex(ctx, points, 4, corner_radius, color, position, placement, result);
}

void dc_app_draw_image_ex(DcAppDrawContext *ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint, DcAppPlacement placement, DcAppDrawResult *result) {
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

    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data || texture_id >= (DcAppTextureId)sbcount(app_data->sb_textures)) return false;

    *out = app_data->sb_textures[texture_id].bind_group_handle.uData;
    return true;
}

void dc_app_draw_image_quad(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area) {
    if (!ctx) return;
    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    DcAppVec2 points[4] = {p0, p1, p2, p3};
    plVec2 draw_points[4];
    dc_app_draw_resolve_points(ctx, points, 4, position, placement, draw_points, out_area);
    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type        = _DC_APP_DRAW_COMMAND_IMAGE_QUAD,
        .sb_points   = draw_points,
        .point_count = 4,
        .texture_id  = texture_id,
    });

    _ext_dc_draw->add_image_quad(dc_app_draw_batch_get_2d(app_data), texture_id, draw_points[0], draw_points[1], draw_points[2], draw_points[3]);
}

void dc_app_draw_image_quad_uv(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area) {
    if (!ctx) return;
    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    DcAppVec2 points[4] = {p0, p1, p2, p3};
    plVec2 draw_points[4];
    dc_app_draw_resolve_points(ctx, points, 4, position, placement, draw_points, out_area);
    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type        = _DC_APP_DRAW_COMMAND_IMAGE_QUAD_UV,
        .sb_points   = draw_points,
        .point_count = 4,
        .texture_id  = texture_id,
        .uv0         = uv0,
        .uv1         = uv1,
        .uv2         = uv2,
        .uv3         = uv3,
        .tint        = tint,
    });

    _ext_dc_draw->add_image_quad_ex(dc_app_draw_batch_get_2d(app_data), texture_id, draw_points[0], draw_points[1], draw_points[2], draw_points[3],
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
    if (!ctx || radius.x <= 0.0f || radius.y <= 0.0f) return;

    if (placement.local_align_x == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_x = DC_APP_ALIGN_TYPE_CENTER;
    if (placement.local_align_y == DC_APP_ALIGN_TYPE_UNDEFINED) placement.local_align_y = DC_APP_ALIGN_TYPE_MIDDLE;

    enum { SEGMENTS = 64 };
    DcAppVec2 points[SEGMENTS];

    for (int i = 0; i < SEGMENTS; i++) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)SEGMENTS;
        points[i] = (DcAppVec2){radius.x + cosf(theta) * radius.x, radius.y + sinf(theta) * radius.y};
    }

    dc_app_draw_polygon_filled_ex(ctx, points, SEGMENTS, color, center, placement, result);
}

DcAppVec2 dc_app_draw_text_size(DcAppDrawContext *ctx, const char *text, DcAppTextStyle style) {
    if (!ctx || !text) return (DcAppVec2){0.0f, 0.0f};

    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return (DcAppVec2){0.0f, 0.0f};

    dcDrawTextOptions options = _text_options(app_data, style);
    plVec2 size = _ext_dc_draw->calculate_text_size(text, options);
    if (options.fSize > 0.0f) {
        size.y = options.fSize;
    }
    return (DcAppVec2){size.x, size.y};
}

void dc_app_draw_text_ex(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style, DcAppPlacement placement, DcAppDrawResult *result) {
    if (!ctx || !text) return;
    DcAppDrawArea *out_area = _draw_result_area(result);

    _AppData *app_data = (_AppData *)ctx->_runtime;
    if (!app_data) return;

    DcAppVec2 size = dc_app_draw_text_size(ctx, text, style);
    if (size.x == 0.0f || size.y == 0.0f) return;

    plVec2 points[4];
    _resolve_rect_points(ctx, size, position, placement, points, out_area);

    dcDrawTextOptions options = _text_options(app_data, style);
    options.tTransform = _text_transform(ctx, size, position, placement);

    _record_stencil_add_command(ctx, &(_DcAppDrawCommand){
        .type         = _DC_APP_DRAW_COMMAND_TEXT,
        .text         = (char *)text,
        .text_options = options,
    });

    _ext_dc_draw->add_text(dc_app_draw_batch_get_2d(app_data), (plVec2){0.0f, 0.0f}, text, options);
}

//-----------------------------------------------------------------------------
// [SECTION] Internal XML/node draw helpers
//-----------------------------------------------------------------------------

plVec2 dc_app_draw_text_options_size(const char *text, dcDrawTextOptions options) {
    if (!text) return (plVec2){0.0f, 0.0f};
    return _ext_dc_draw->calculate_text_size(text, options);
}

void dc_app_draw_text_options(_AppData *app_data, const char *text, dcDrawTextOptions options) {
    if (!app_data || !text) return;
    _ext_dc_draw->add_text(dc_app_draw_batch_get_2d(app_data), (plVec2){0.0f, 0.0f}, text, options);
}

void dc_app_draw_3d_sphere_textured(_AppData *app_data, uint32_t texture_id, plSphere sphere, const plMat4 *transform, uint32_t color) {
    if (!app_data || !transform) return;
    _ext_dc_draw->add_3d_sphere_textured(dc_app_draw_batch_get_3d(app_data), texture_id, sphere, transform, 32, 32, color);
}

void dc_app_draw_3d_sphere_filled(_AppData *app_data, plSphere sphere, uint32_t color) {
    if (!app_data) return;
    _ext_dc_draw->add_3d_sphere_filled(dc_app_draw_batch_get_3d(app_data), sphere, 32, 32, (dcDrawSolidOptions){.uColor = color});
}

void dc_app_draw_planet_polygon_filled(plPlanetView *view, plVec3 *points, uint32_t point_count, uint32_t color) {
    if (!view || !points || point_count < 3) return;
    _ext_planet->draw_polygon_filled(view, points, point_count, color);
}

void dc_app_draw_planet_polygon(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color) {
    if (!view || !points || point_count < 3) return;
    _ext_planet->draw_polygon(view, points, point_count, line_width, color);
}

void dc_app_draw_planet_line(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color) {
    if (!view || !points || point_count < 2) return;
    _ext_planet->draw_line(view, points, point_count, line_width, color);
}

void dc_app_draw_planet_sphere(plPlanetView *view, float lon, float lat, float height, float radius, uint32_t color) {
    if (!view || radius <= 0.0f) return;
    _ext_planet->draw_sphere(view, lon, lat, height, radius, color);
}

void dc_app_draw_planet_text(plPlanetView *view, plCamera *camera, plVec3 position, const char *text, float size, uint32_t color) {
    if (!view || !camera || !text) return;
    _ext_planet->draw_text(view, camera, position, text, size, color);
}

//-----------------------------------------------------------------------------
// [SECTION] draw batch utils
//-----------------------------------------------------------------------------

void dc_app_draw_batch_reset(_AppData *app_data) {
    // clear the batches array (doesn't free memory, just resets count)
    sbclear(app_data->sb_draw_batches);

    // reset pool indices
    app_data->draw_list_2d_index = 0;
    app_data->draw_list_3d_index = 0;
}

dcDrawLayer2D *dc_app_draw_batch_get_2d(_AppData *app_data) {
    // if last batch is already 2D, return the same layer
    int count = sbcount(app_data->sb_draw_batches);
    if (count > 0 && app_data->sb_draw_batches[count - 1].type == DRAW_BATCH_TYPE_2D) {
        dcDrawLayer2D *layer = app_data->sb_draw_batches[count - 1].draw_list_2d.layer;

        // inject stencil override on phase change
        if (app_data->stencil_2d_dirty) {
            _ext_dc_draw_backend->set_shader(layer, app_data->active_2d_shader_override, app_data->active_sdf_shader_override);
            app_data->stencil_2d_dirty = false;
        }
        return layer;
    }

    // grow pool if needed - request from extension
    int pool_size = sbcount(app_data->sb_draw_list_2d_pool);
    if (app_data->draw_list_2d_index >= pool_size) {
        dcDrawList2D  *new_draw_list = _ext_dc_draw->request_2d_drawlist();
        dcDrawLayer2D *new_layer     = _ext_dc_draw->request_2d_layer(new_draw_list);
        _DrawList2D    new_entry     = {.draw_list = new_draw_list, .layer = new_layer};
        sbpush(app_data->sb_draw_list_2d_pool, new_entry);
    }

    // get draw list + layer from pool
    _DrawList2D *draw_list_2d = &app_data->sb_draw_list_2d_pool[app_data->draw_list_2d_index];
    app_data->draw_list_2d_index++;

    // add batch entry
    _DrawBatch batch = {
        .type         = DRAW_BATCH_TYPE_2D,
        .draw_list_2d = *draw_list_2d};
    sbpush(app_data->sb_draw_batches, batch);

    // inject stencil override if active (new batch after batch break)
    if (app_data->stencil_2d_dirty || app_data->active_2d_shader_override || app_data->active_sdf_shader_override) {
        _ext_dc_draw_backend->set_shader(draw_list_2d->layer, app_data->active_2d_shader_override, app_data->active_sdf_shader_override);
        app_data->stencil_2d_dirty = false;
    }

    return draw_list_2d->layer;
}

dcDrawList3D *dc_app_draw_batch_get_3d(_AppData *app_data) {
    // if last batch is already 3D, return the same draw list
    int count = sbcount(app_data->sb_draw_batches);
    if (count > 0 && app_data->sb_draw_batches[count - 1].type == DRAW_BATCH_TYPE_3D) {
        dcDrawList3D *draw_list = app_data->sb_draw_batches[count - 1].draw_list_3d;

        // inject stencil override on phase change
        if (app_data->stencil_3d_dirty) {
            _ext_dc_draw_backend->set_3d_shader(draw_list,
                                             app_data->active_3d_solid_shader_override,
                                             app_data->active_3d_textured_shader_override);
            app_data->stencil_3d_dirty = false;
        }
        return draw_list;
    }

    // grow pool if needed - request from extension
    int pool_size = sbcount(app_data->sb_draw_list_3d_pool);
    if (app_data->draw_list_3d_index >= pool_size) {
        dcDrawList3D *new_list = _ext_dc_draw->request_3d_drawlist();
        sbpush(app_data->sb_draw_list_3d_pool, new_list);
    }

    // get draw list from pool
    dcDrawList3D *draw_list = app_data->sb_draw_list_3d_pool[app_data->draw_list_3d_index];
    app_data->draw_list_3d_index++;

    // add batch entry
    _DrawBatch batch = {
        .type         = DRAW_BATCH_TYPE_3D,
        .draw_list_3d = draw_list};
    sbpush(app_data->sb_draw_batches, batch);

    // inject stencil override if active (new batch after batch break)
    if (app_data->stencil_3d_dirty || app_data->active_3d_solid_shader_override || app_data->active_3d_textured_shader_override) {
        _ext_dc_draw_backend->set_3d_shader(draw_list,
                                         app_data->active_3d_solid_shader_override,
                                         app_data->active_3d_textured_shader_override);
        app_data->stencil_3d_dirty = false;
    }

    return draw_list;
}

static void _set_stencil_phase(_AppData *app_data, int depth, _DcAppStencilPhase phase) {
    if (!app_data || depth <= 0 || depth > DC_STENCIL_MAX_DEPTH) return;

    switch (phase) {
        case _DC_APP_STENCIL_PHASE_CREATE:
            app_data->active_2d_shader_override          = &app_data->stencil_create_2d_shader;
            app_data->active_sdf_shader_override         = &app_data->stencil_create_sdf_shader;
            app_data->active_3d_solid_shader_override    = &app_data->stencil_create_3d_solid_shader;
            app_data->active_3d_textured_shader_override = &app_data->stencil_create_3d_textured_shader;
            break;
        case _DC_APP_STENCIL_PHASE_REMOVE:
            app_data->active_2d_shader_override          = &app_data->stencil_remove_2d_shader;
            app_data->active_sdf_shader_override         = &app_data->stencil_remove_sdf_shader;
            app_data->active_3d_solid_shader_override    = &app_data->stencil_remove_3d_solid_shader;
            app_data->active_3d_textured_shader_override = &app_data->stencil_remove_3d_textured_shader;
            break;
        case _DC_APP_STENCIL_PHASE_DRAW:
            app_data->active_2d_shader_override          = &app_data->stencil_draw_2d_shader[depth - 1];
            app_data->active_sdf_shader_override         = &app_data->stencil_draw_sdf_shader[depth - 1];
            app_data->active_3d_solid_shader_override    = &app_data->stencil_draw_3d_solid_shader[depth - 1];
            app_data->active_3d_textured_shader_override = &app_data->stencil_draw_3d_textured_shader[depth - 1];
            break;
        case _DC_APP_STENCIL_PHASE_CLEANUP:
            app_data->active_2d_shader_override          = &app_data->stencil_cleanup_2d_shader;
            app_data->active_sdf_shader_override         = &app_data->stencil_cleanup_sdf_shader;
            app_data->active_3d_solid_shader_override    = &app_data->stencil_cleanup_3d_solid_shader;
            app_data->active_3d_textured_shader_override = &app_data->stencil_cleanup_3d_textured_shader;
            break;
        default:
            return;
    }

    app_data->stencil_phase    = phase;
    app_data->stencil_2d_dirty = true;
    app_data->stencil_3d_dirty = true;
}

static void _restore_stencil_phase(_AppData *app_data, _DcAppStencilPhase phase) {
    if (!app_data) return;

    if (app_data->stencil_depth > 0) {
        if (phase == _DC_APP_STENCIL_PHASE_NONE || phase == _DC_APP_STENCIL_PHASE_CLEANUP) {
            phase = _DC_APP_STENCIL_PHASE_DRAW;
        }
        _set_stencil_phase(app_data, app_data->stencil_depth, phase);
    } else {
        app_data->active_2d_shader_override          = NULL;
        app_data->active_sdf_shader_override         = NULL;
        app_data->active_3d_solid_shader_override    = NULL;
        app_data->active_3d_textured_shader_override = NULL;
        app_data->stencil_phase                      = _DC_APP_STENCIL_PHASE_NONE;
        app_data->stencil_2d_dirty                   = true;
        app_data->stencil_3d_dirty                   = true;
    }
}

static _DcAppContainerData *_container_data(DcAppDrawContext *ctx) {
    if (!ctx) return NULL;
    if (!ctx->_container_data) {
        ctx->_container_data = PL_ALLOC(sizeof(_DcAppContainerData));
        if (!ctx->_container_data) return NULL;
        memset(ctx->_container_data, 0, sizeof(_DcAppContainerData));
    }
    return (_DcAppContainerData *)ctx->_container_data;
}

static _DcAppStencilRecorder *_stencil_recorder(DcAppDrawContext *ctx) {
    if (!ctx) return NULL;
    if (!ctx->_stencil_data) {
        ctx->_stencil_data = PL_ALLOC(sizeof(_DcAppStencilRecorder));
        if (!ctx->_stencil_data) return NULL;
        memset(ctx->_stencil_data, 0, sizeof(_DcAppStencilRecorder));
    }
    return (_DcAppStencilRecorder *)ctx->_stencil_data;
}

static void _record_stencil_add_command(DcAppDrawContext *ctx, const _DcAppDrawCommand *command) {
    if (!ctx || !command) return;

    _DcAppStencilRecorder *recorder = (_DcAppStencilRecorder *)ctx->_stencil_data;
    if (!recorder || recorder->is_replaying || recorder->phase != _DC_APP_STENCIL_PHASE_CREATE || sbcount(recorder->sb_frames) == 0) return;

    _DcAppDrawCommand copy = *command;
    copy.sb_points         = NULL;
    for (uint32_t ii = 0; ii < command->point_count; ii++) {
        sbpush(copy.sb_points, command->sb_points[ii]);
    }
    if (command->text) {
        size_t len = strlen(command->text) + 1;
        copy.text = PL_ALLOC(len);
        if (!copy.text) {
            sbfree(copy.sb_points);
            return;
        }
        memcpy(copy.text, command->text, len);
        copy.text_options.pcTextEnd = NULL;
    }

    _DcAppStencilFrame *frame = &recorder->sb_frames[sbcount(recorder->sb_frames) - 1];
    sbpush(frame->sb_add_commands, copy);
}

static void _replay_stencil_command(_AppData *app_data, const _DcAppDrawCommand *command) {
    if (!app_data || !command) return;

    dcDrawLayer2D *layer = dc_app_draw_batch_get_2d(app_data);
    switch (command->type) {
        case _DC_APP_DRAW_COMMAND_LINES:
            if (command->point_count < 2) return;
            _ext_dc_draw->add_lines(layer, command->sb_points, command->point_count, (dcDrawLineOptions){
                .uColor       = PL_COLOR_32_RGBA(command->stroke.color.r, command->stroke.color.g, command->stroke.color.b, command->stroke.color.a),
                .fThickness   = command->stroke.width * DCAPP_LINE_WIDTH_FACTOR,
                .uDashPattern = command->stroke.pattern,
            });
            break;

        case _DC_APP_DRAW_COMMAND_TRIANGLES_FILLED:
            if (command->point_count < 3) return;
            _ext_dc_draw->add_triangles_filled(layer, command->sb_points, command->point_count, (dcDrawSolidOptions){
                .uColor = PL_COLOR_32_RGBA(command->color.r, command->color.g, command->color.b, command->color.a),
            });
            break;

        case _DC_APP_DRAW_COMMAND_POLYGON:
            if (command->point_count < 3) return;
            if (command->corner_radius > 0.0f) {
                _ext_dc_draw->add_polygon_rounded(layer, command->sb_points, command->point_count, command->corner_radius, 8, (dcDrawLineOptions){
                    .uColor       = PL_COLOR_32_RGBA(command->stroke.color.r, command->stroke.color.g, command->stroke.color.b, command->stroke.color.a),
                    .fThickness   = command->stroke.width * DCAPP_LINE_WIDTH_FACTOR,
                    .uDashPattern = command->stroke.pattern,
                });
            } else {
                _ext_dc_draw->add_polygon(layer, command->sb_points, command->point_count, (dcDrawLineOptions){
                    .uColor       = PL_COLOR_32_RGBA(command->stroke.color.r, command->stroke.color.g, command->stroke.color.b, command->stroke.color.a),
                    .fThickness   = command->stroke.width * DCAPP_LINE_WIDTH_FACTOR,
                    .uDashPattern = command->stroke.pattern,
                });
            }
            break;

        case _DC_APP_DRAW_COMMAND_POLYGON_FILLED:
            if (command->point_count < 3) return;
            if (command->corner_radius > 0.0f) {
                _ext_dc_draw->add_convex_polygon_rounded_filled(layer, command->sb_points, command->point_count, command->corner_radius, 8, (dcDrawSolidOptions){
                    .uColor = PL_COLOR_32_RGBA(command->color.r, command->color.g, command->color.b, command->color.a),
                });
            } else {
                _ext_dc_draw->add_convex_polygon_filled(layer, command->sb_points, command->point_count, (dcDrawSolidOptions){
                    .uColor = PL_COLOR_32_RGBA(command->color.r, command->color.g, command->color.b, command->color.a),
                });
            }
            break;

        case _DC_APP_DRAW_COMMAND_IMAGE_QUAD:
            if (command->point_count < 4) return;
            _ext_dc_draw->add_image_quad(layer, command->texture_id, command->sb_points[0], command->sb_points[1], command->sb_points[2], command->sb_points[3]);
            break;

        case _DC_APP_DRAW_COMMAND_IMAGE_QUAD_UV:
            if (command->point_count < 4) return;
            _ext_dc_draw->add_image_quad_ex(layer, command->texture_id,
                                            command->sb_points[0], command->sb_points[1], command->sb_points[2], command->sb_points[3],
                                            (plVec2){command->uv0.x, command->uv0.y},
                                            (plVec2){command->uv1.x, command->uv1.y},
                                            (plVec2){command->uv2.x, command->uv2.y},
                                            (plVec2){command->uv3.x, command->uv3.y},
                                            PL_COLOR_32_RGBA(command->tint.r, command->tint.g, command->tint.b, command->tint.a));
            break;

        case _DC_APP_DRAW_COMMAND_TEXT:
            if (!command->text) return;
            _ext_dc_draw->add_text(layer, (plVec2){0.0f, 0.0f}, command->text, command->text_options);
            break;

        default:
            break;
    }
}

static void _free_stencil_command(_DcAppDrawCommand *command) {
    if (!command) return;
    sbfree(command->sb_points);
    command->sb_points = NULL;
    if (command->text) {
        PL_FREE(command->text);
        command->text = NULL;
    }
}

void dc_app_draw_context_cleanup(DcAppDrawContext *ctx) {
    if (!ctx) return;

    if (ctx->_container_data) {
        PL_FREE(ctx->_container_data);
        ctx->_container_data = NULL;
    }

    _DcAppStencilRecorder *recorder = (_DcAppStencilRecorder *)ctx->_stencil_data;
    while (recorder && sbcount(recorder->sb_frames) > 0) {
        int frame_count = sbcount(recorder->sb_frames);
        dc_app_draw_stencil_end(ctx);
        recorder = (_DcAppStencilRecorder *)ctx->_stencil_data;
        if (recorder && sbcount(recorder->sb_frames) >= frame_count) {
            DC_LOG_WARN("DrawFunction", "Stencil cleanup stopped because stencil state did not advance");
            break;
        }
    }

    recorder = (_DcAppStencilRecorder *)ctx->_stencil_data;
    if (recorder) {
        for (int ii = 0; ii < sbcount(recorder->sb_frames); ii++) {
            _DcAppStencilFrame *frame = &recorder->sb_frames[ii];
            for (int jj = 0; jj < sbcount(frame->sb_add_commands); jj++) {
                _free_stencil_command(&frame->sb_add_commands[jj]);
            }
            sbfree(frame->sb_add_commands);
        }
        sbfree(recorder->sb_frames);
        PL_FREE(recorder);
        ctx->_stencil_data = NULL;
    }
}
