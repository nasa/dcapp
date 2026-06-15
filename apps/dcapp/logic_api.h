#ifndef _DCAPP_LOGIC_API_H_
#define _DCAPP_LOGIC_API_H_

#include "app/enums.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct __AppData DcAppContext;
typedef struct _DcAppDrawContext DcAppDrawContext;
typedef struct _DcAppPlanet *DcAppPlanetHandle;
typedef struct _DcAppPlanetView *DcAppPlanetViewHandle;
typedef struct _DcAppDrawPlanetView *DcAppDrawPlanetViewHandle;

typedef uint32_t DcAppTextureId;

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
    struct { float roll, pitch, yaw; };
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

typedef struct _DcAppPlanetCreateInfo {
    const char *data_path;
    uint32_t mesh_cache_size; // bytes, 0 = renderer default
} DcAppPlanetCreateInfo;

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
    void *_planet_view_data;
    bool _owns_stencil_data;
    int _stencil_base_depth;
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
    const DcAppDrawArea *(*get_area)(DcAppDrawContext *draw_ctx);

    // Basic draw functions.
    void (*line)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke);
    void (*polyline)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
    void (*polygon)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
    void (*polygon_filled)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
    void (*rounded_polygon)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke);
    void (*rounded_polygon_filled)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color);
    void (*quad)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke);
    void (*quad_filled)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color);
    void (*rounded_quad)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke);
    void (*rounded_quad_filled)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color);
    void (*image)(DcAppDrawContext *draw_ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint);
    void (*rect)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke);
    void (*rect_filled)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color);
    void (*rounded_rect)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke);
    void (*rounded_rect_filled)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color);
    void (*circle)(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppStroke stroke);
    void (*circle_filled)(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppVec4 color);
    void (*ellipse)(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppStroke stroke);
    void (*ellipse_filled)(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppVec4 color);
    DcAppVec2 (*text_size)(DcAppDrawContext *draw_ctx, const char *text, DcAppTextStyle style);
    void (*text)(DcAppDrawContext *draw_ctx, DcAppVec2 position, const char *text, DcAppTextStyle style);

    // Extended draw functions with placement and result metadata.
    void (*line_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*polyline_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*polygon_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*polygon_filled_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_polygon_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_polygon_filled_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*quad_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*quad_filled_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_quad_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_quad_filled_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*image_ex)(DcAppDrawContext *draw_ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rect_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rect_filled_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_rect_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_rect_filled_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*circle_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*circle_filled_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*ellipse_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
    void (*ellipse_filled_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
    void (*text_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 position, const char *text, DcAppTextStyle style, DcAppPlacement placement, DcAppDrawResult *result);

    // Container helpers.
    bool (*container_push)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size);
    bool (*container_push_ex)(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size, DcAppPlacement placement, DcAppDrawResult *result);
    bool (*container_push_area)(DcAppDrawContext *draw_ctx, const DcAppDrawArea *area);
    void (*container_pop)(DcAppDrawContext *draw_ctx);

    // Stencil helpers.
    bool (*stencil_begin)(DcAppDrawContext *draw_ctx);
    void (*stencil_add)(DcAppDrawContext *draw_ctx);
    void (*stencil_remove)(DcAppDrawContext *draw_ctx);
    void (*stencil_draw)(DcAppDrawContext *draw_ctx);
    void (*stencil_end)(DcAppDrawContext *draw_ctx);

    // draws planet views and overlays through dcapp handles.
    DcAppDrawPlanetViewHandle (*planet_view_geodetic)(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, double lat, double lon, double elevation, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
    DcAppDrawPlanetViewHandle (*planet_view_cartesian)(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, DcAppVec3 camera_position, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
    void (*planet_sphere_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, double radius, DcAppVec4 color);
    void (*planet_sphere_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, float radius, DcAppVec4 color);
    void (*planet_text_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, const char *text, float size, DcAppVec4 color);
    void (*planet_text_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, const char *text, float size, DcAppVec4 color);
} DcAppDrawApi;

typedef struct _DcAppMouseApi {
    // Basic mouse hit registration.
    void (*rect)(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 position, DcAppVec2 size);
    void (*circle)(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, float radius);
    void (*ellipse)(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, DcAppVec2 radius);
    void (*polygon)(DcAppDrawContext *draw_ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position);

    // Extended mouse hit registration with placement.
    void (*rect_ex)(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement);
    void (*circle_ex)(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, float radius, DcAppPlacement placement);
    void (*ellipse_ex)(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, DcAppVec2 radius, DcAppPlacement placement);
    void (*polygon_ex)(DcAppDrawContext *draw_ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement);

    // Event queries.
    bool (*hovered)(DcAppDrawContext *draw_ctx, const char *id);
    bool (*pressed)(DcAppDrawContext *draw_ctx, const char *id);
    bool (*released)(DcAppDrawContext *draw_ctx, const char *id);
    bool (*active)(DcAppDrawContext *draw_ctx, const char *id);
    bool (*clicked)(DcAppDrawContext *draw_ctx, const char *id);

    // Current mouse state in the draw context's local space.
    bool (*down)(DcAppDrawContext *draw_ctx);
    const DcAppMouse *(*get_state)(DcAppDrawContext *draw_ctx);
} DcAppMouseApi;

typedef struct _DcAppTextureApi {
    DcAppTextureId (*load_image)(DcAppContext *app_ctx, const char *path, DcAppVec2 *out_size);
    bool (*get_size)(DcAppContext *app_ctx, DcAppTextureId texture_id, DcAppVec2 *out_size);
} DcAppTextureApi;

typedef struct _DcAppPlanetApi {
    // planet resources live until app shutdown.
    DcAppPlanetHandle (*get_planet_by_id)(DcAppContext *app_ctx, const char *id);
    DcAppPlanetHandle (*create_planet)(DcAppContext *app_ctx, DcAppPlanetCreateInfo info);
    // fails if id already exists.
    DcAppPlanetHandle (*create_planet_with_id)(DcAppContext *app_ctx, const char *id, DcAppPlanetCreateInfo info);
    bool (*set_texture_geodetic)(DcAppContext *app_ctx, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel);
    bool (*set_texture_cartesian)(DcAppContext *app_ctx, DcAppPlanetHandle planet, const char *path, DcAppVec3 position, float meters_per_pixel);
    DcAppPlanetViewHandle (*create_geodetic_view)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
    DcAppPlanetViewHandle (*create_cartesian_view)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
} DcAppPlanetApi;

typedef void *(*DcAppGetVariableFn)(DcAppContext *app_ctx, const char *name);

typedef struct _DcAppInit {
    uint32_t size;
    uint32_t version;
    DcAppContext *app_ctx;
    DcAppGetVariableFn get_variable;
    const DcAppDrawApi *draw;
    const DcAppMouseApi *mouse;
    const DcAppTextureApi *texture;
    const DcAppPlanetApi *planet;
} DcAppInit;

#endif
