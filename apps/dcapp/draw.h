#ifndef _DCAPP_DRAW_H_
#define _DCAPP_DRAW_H_

#include "dcapp.h"

#ifndef DCAPP_LINE_WIDTH_FACTOR
#define DCAPP_LINE_WIDTH_FACTOR 2.0f
#endif

#ifndef DCAPP_DRAW_CONTEXT_STACK_MAX
#define DCAPP_DRAW_CONTEXT_STACK_MAX 64
#endif

#ifndef DCAPP_DRAW_POINT_COUNT_MAX
#define DCAPP_DRAW_POINT_COUNT_MAX 65536
#endif

// DrawFunction ABI mirrored in generated logic/dcapp.h
typedef struct _DcAppDrawContext DcAppDrawContext;

typedef union _DcAppVec2 {
    struct { float x, y; };
    struct { float r, g; };
    struct { float u, v; };
    float d[2];
} DcAppVec2;

typedef union _DcAppVec3 {
    struct { float x, y, z; };
    struct { float r, g, b; };
    struct { float u, v, __; };
    struct { DcAppVec2 xy; float ignore0_; };
    struct { DcAppVec2 rg; float ignore1_; };
    struct { DcAppVec2 uv; float ignore2_; };
    struct { float ignore3_; DcAppVec2 yz; };
    struct { float ignore4_; DcAppVec2 gb; };
    struct { float ignore5_; DcAppVec2 v__; };
    float d[3];
} DcAppVec3;

typedef union _DcAppVec4 {
    struct {
        union {
            DcAppVec3 xyz;
            struct { float x, y, z; };
        };
        float w;
    };
    struct {
        union {
            DcAppVec3 rgb;
            struct { float r, g, b; };
        };
        float a;
    };
    struct {
        DcAppVec2 xy;
        float ignored0_, ignored1_;
    };
    struct {
        float ignored2_;
        DcAppVec2 yz;
        float ignored3_;
    };
    struct {
        float ignored4_, ignored5_;
        DcAppVec2 zw;
    };
    float d[4];
} DcAppVec4;

typedef struct _DcAppStroke {
    DcAppVec4 color;
    float width;
    uint8_t pattern;
} DcAppStroke;

typedef struct _DcAppTextStyle {
    DcAppVec4 color;
    float size;
    float wrap;
} DcAppTextStyle;

typedef struct _DcAppPlacement {
    float rotation;
    DcAppAlignType parent_align_x;
    DcAppAlignType parent_align_y;
    DcAppAlignType local_align_x;
    DcAppAlignType local_align_y;
    DcAppAlignType pivot_align_x;
    DcAppAlignType pivot_align_y;
    float pivot_x;
    float pivot_y;
} DcAppPlacement;

typedef struct _DcAppMouse {
    float x;
    float y;
    bool position_valid;
    bool pressed;
    bool released;
    bool down;
} DcAppMouse;

typedef struct _DcAppDrawArea {
    float position[2];
    float dimensions[2];
    float transform[16];
} DcAppDrawArea;

typedef struct _DcAppDrawResult {
    DcAppDrawArea area;
} DcAppDrawResult;

struct _DcAppDrawContext {
    void *_runtime;
    void *_container_data;
    void *_stencil_data;
    DcAppDrawArea area;
    DcAppMouse mouse;
};

typedef struct _DcAppDrawFuncArg {
    DcValueType type;
    const char *value_string;
    int value_integer;
    double value_double;
    bool value_boolean;
} DcAppDrawFuncArg;

typedef struct _DcAppDrawFuncArgs {
    uint32_t count;
    const DcAppDrawFuncArg *values;
} DcAppDrawFuncArgs;

typedef struct _DcAppDrawApi {
    // Current draw area.
    const DcAppDrawArea *(*get_area)(DcAppDrawContext *ctx);

    // Basic draw functions.
    void (*line)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke);
    void (*polyline)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
    void (*polygon)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
    void (*polygon_filled)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
    void (*rounded_polygon)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke);
    void (*rounded_polygon_filled)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color);
    void (*quad)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke);
    void (*quad_filled)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color);
    void (*rounded_quad)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke);
    void (*rounded_quad_filled)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color);
    void (*image)(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint);
    void (*rect)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke);
    void (*rect_filled)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color);
    void (*rounded_rect)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke);
    void (*rounded_rect_filled)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color);
    void (*circle)(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppStroke stroke);
    void (*circle_filled)(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppVec4 color);
    DcAppVec2 (*text_size)(DcAppDrawContext *ctx, const char *text, DcAppTextStyle style);
    void (*text)(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style);

    // Extended draw functions with placement and result metadata.
    void (*line_ex)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*polyline_ex)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*polygon_ex)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*polygon_filled_ex)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_polygon_ex)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_polygon_filled_ex)(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*quad_ex)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*quad_filled_ex)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_quad_ex)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_quad_filled_ex)(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*image_ex)(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rect_ex)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rect_filled_ex)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_rect_ex)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_rect_filled_ex)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*circle_ex)(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*circle_filled_ex)(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*text_ex)(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style, DcAppPlacement placement, DcAppDrawResult *result);

    // Container helpers.
    bool (*container_push)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size);
    bool (*container_push_ex)(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size, DcAppPlacement placement, DcAppDrawResult *result);
    bool (*container_push_area)(DcAppDrawContext *ctx, const DcAppDrawArea *area);
    void (*container_pop)(DcAppDrawContext *ctx);

    // Stencil helpers.
    bool (*stencil_begin)(DcAppDrawContext *ctx);
    void (*stencil_add)(DcAppDrawContext *ctx);
    void (*stencil_remove)(DcAppDrawContext *ctx);
    void (*stencil_draw)(DcAppDrawContext *ctx);
    void (*stencil_end)(DcAppDrawContext *ctx);
} DcAppDrawApi;

typedef struct _DcAppMouseApi {
    const DcAppMouse *(*get_state)(DcAppDrawContext *ctx);
    void (*rect)(DcAppDrawContext *ctx, const char *id, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement);
    void (*circle)(DcAppDrawContext *ctx, const char *id, DcAppVec2 center, float radius, DcAppPlacement placement);
    void (*polygon)(DcAppDrawContext *ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement);
    bool (*hovered)(DcAppDrawContext *ctx, const char *id);
    bool (*pressed)(DcAppDrawContext *ctx, const char *id);
    bool (*released)(DcAppDrawContext *ctx, const char *id);
    bool (*down)(DcAppDrawContext *ctx, const char *id);
    bool (*active)(DcAppDrawContext *ctx, const char *id);
    bool (*clicked)(DcAppDrawContext *ctx, const char *id);
} DcAppMouseApi;

typedef void *(*DcAppGetVariableFn)(void *user_data, const char *name);

typedef struct _DcAppInit {
    uint32_t size;
    uint32_t version;
    void *user_data;
    DcAppGetVariableFn get_variable;
    const DcAppDrawApi *draw;
    const DcAppMouseApi *mouse;
} DcAppInit;

typedef struct _DcAppStencilHandler {
    int depth;
} DcAppStencilHandler;

// draw batch utils
void           dc_app_draw_batch_reset(_AppData *app_data);
dcDrawLayer2D *dc_app_draw_batch_get_2d(_AppData *app_data);
dcDrawList3D  *dc_app_draw_batch_get_3d(_AppData *app_data);

// DrawFunction context and placement helpers
DcAppDrawContext dc_app_draw_context(_AppData *app_data, _NodeIndex node_index, plVec2 parent_position, plVec2 parent_dimensions, const plMat4 *parent_transform);
void             dc_app_draw_context_cleanup(DcAppDrawContext *ctx);

// DrawFunction primitive API
const DcAppDrawArea *dc_app_draw_get_area(DcAppDrawContext *ctx);
void dc_app_draw_line(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke);
void dc_app_draw_line_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polyline(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
void dc_app_draw_polyline_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_triangles_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polygon(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
void dc_app_draw_polygon_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polygon_filled(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
void dc_app_draw_polygon_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_polygon(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_polygon_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_polygon_filled(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_polygon_filled_ex(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_quad(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke);
void dc_app_draw_quad_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_quad_filled(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color);
void dc_app_draw_quad_filled_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_quad(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_quad_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_quad_filled(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_quad_filled_ex(DcAppDrawContext *ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_image(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint);
void dc_app_draw_image_ex(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_image_quad(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area);
void dc_app_draw_image_quad_uv(DcAppDrawContext *ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area);
void dc_app_draw_rect(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke);
void dc_app_draw_rect_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rect_filled(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color);
void dc_app_draw_rect_filled_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_rect(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_rect_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_rect_filled(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_rect_filled_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_circle(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppStroke stroke);
void dc_app_draw_circle_ex(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_circle_filled(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppVec4 color);
void dc_app_draw_circle_filled_ex(DcAppDrawContext *ctx, DcAppVec2 center, float radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
DcAppVec2 dc_app_draw_text_size(DcAppDrawContext *ctx, const char *text, DcAppTextStyle style);
void dc_app_draw_text(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style);
void dc_app_draw_text_ex(DcAppDrawContext *ctx, DcAppVec2 position, const char *text, DcAppTextStyle style, DcAppPlacement placement, DcAppDrawResult *result);

// DrawFunction utility API
void dc_app_draw_resolve_points(DcAppDrawContext *ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement, plVec2 *out, DcAppDrawArea *out_area);
bool dc_app_draw_container_push(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size);
bool dc_app_draw_container_push_ex(DcAppDrawContext *ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size, DcAppPlacement placement, DcAppDrawResult *result);
bool dc_app_draw_container_push_area(DcAppDrawContext *ctx, const DcAppDrawArea *area);
void dc_app_draw_container_pop(DcAppDrawContext *ctx);
bool dc_app_draw_stencil_begin(DcAppDrawContext *ctx);
void dc_app_draw_stencil_add(DcAppDrawContext *ctx);
void dc_app_draw_stencil_remove(DcAppDrawContext *ctx);
void dc_app_draw_stencil_draw(DcAppDrawContext *ctx);
void dc_app_draw_stencil_end(DcAppDrawContext *ctx);

// DrawFunction mouse registration and event API
const DcAppMouse *dc_app_mouse_get_state(DcAppDrawContext *ctx);
void dc_app_mouse_rect(DcAppDrawContext *ctx, const char *id, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement);
void dc_app_mouse_circle(DcAppDrawContext *ctx, const char *id, DcAppVec2 center, float radius, DcAppPlacement placement);
void dc_app_mouse_polygon(DcAppDrawContext *ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement);
bool dc_app_mouse_hovered(DcAppDrawContext *ctx, const char *id);
bool dc_app_mouse_pressed(DcAppDrawContext *ctx, const char *id);
bool dc_app_mouse_released(DcAppDrawContext *ctx, const char *id);
bool dc_app_mouse_down(DcAppDrawContext *ctx, const char *id);
bool dc_app_mouse_active(DcAppDrawContext *ctx, const char *id);
bool dc_app_mouse_clicked(DcAppDrawContext *ctx, const char *id);

// stencil state helpers
bool dc_app_draw_stencil_begin_handler(_AppData *app_data, DcAppStencilHandler *handler);
void dc_app_draw_set_stencil_add(_AppData *app_data, const DcAppStencilHandler *handler);
void dc_app_draw_set_stencil_remove(_AppData *app_data, const DcAppStencilHandler *handler);
void dc_app_draw_set_stencil_draw(_AppData *app_data, const DcAppStencilHandler *handler);
void dc_app_draw_set_stencil_cleanup(_AppData *app_data, const DcAppStencilHandler *handler);
void dc_app_draw_stencil_end_handler(_AppData *app_data, const DcAppStencilHandler *handler);

// Internal XML/node draw helpers not exposed through DrawFunction yet
plVec2 dc_app_draw_text_options_size(const char *text, dcDrawTextOptions options);
void   dc_app_draw_text_options(_AppData *app_data, const char *text, dcDrawTextOptions options);
void   dc_app_draw_3d_sphere_textured(_AppData *app_data, uint32_t texture_id, plSphere sphere, const plMat4 *transform, uint32_t color);
void   dc_app_draw_3d_sphere_filled(_AppData *app_data, plSphere sphere, uint32_t color);
void   dc_app_draw_planet_polygon_filled(plPlanetView *view, plVec3 *points, uint32_t point_count, uint32_t color);
void   dc_app_draw_planet_polygon(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color);
void   dc_app_draw_planet_line(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color);
void   dc_app_draw_planet_sphere(plPlanetView *view, float lon, float lat, float height, float radius, uint32_t color);
void   dc_app_draw_planet_text(plPlanetView *view, plCamera *camera, plVec3 position, const char *text, float size, uint32_t color);

const DcAppDrawApi *dc_app_draw_api(void);
const DcAppMouseApi *dc_app_mouse_api(void);

#endif
