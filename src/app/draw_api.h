#ifndef DC_APP_DRAW_API_H
#define DC_APP_DRAW_API_H

#include "app/draw_types.h"
#include "app/planet_api.h"
#include "app/texture_types.h"
#include "app/vector.h"
#include "value_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct DcAppDrawContext DcAppDrawContext;
// This handle is borrowed and valid only within its current draw scope.
typedef struct DcAppDrawPlanetView *DcAppDrawPlanetViewHandle;
typedef struct DcAppStroke DcAppStroke;
typedef struct DcAppTextStyle DcAppTextStyle;
typedef struct DcAppPlacement DcAppPlacement;
typedef struct DcAppMouse DcAppMouse;
typedef struct DcAppDrawArea DcAppDrawArea;
typedef struct DcAppDrawResult DcAppDrawResult;
typedef struct DcAppDrawFuncArg DcAppDrawFuncArg;
typedef struct DcAppDrawFuncArgs DcAppDrawFuncArgs;
typedef struct DcAppDrawApi DcAppDrawApi;
typedef struct DcAppMouseApi DcAppMouseApi;

struct DcAppStroke {
    DcAppVec4 color;
    float width;
    uint8_t pattern;
};

struct DcAppTextStyle {
    DcAppVec4 color;
    float size;
    float wrap;
};

struct DcAppPlacement {
    float rotation;
    DcAppAlignType parent_align_x;
    DcAppAlignType parent_align_y;
    DcAppAlignType local_align_x;
    DcAppAlignType local_align_y;
    DcAppAlignType pivot_align_x;
    DcAppAlignType pivot_align_y;
    float pivot_x;
    float pivot_y;
};

struct DcAppMouse {
    float x;
    float y;
    bool position_valid;
    bool pressed;
    bool released;
    bool down;
};

struct DcAppDrawArea {
    float position[2];
    float dimensions[2];
    float transform[16];
};

// Every draw call that accepts this output clears it before validating inputs.
struct DcAppDrawResult {
    DcAppDrawArea area;
};

struct DcAppDrawFuncArg {
    DcValueType type;
    const char *value_string;
    int value_integer;
    double value_double;
    bool value_boolean;
};

// The values are borrowed for the duration of the DrawFunction callback.
struct DcAppDrawFuncArgs {
    uint32_t count;
    const DcAppDrawFuncArg *values;
};

struct DcAppDrawApi {
    // Current draw area.
    const DcAppDrawArea *(*get_area)(DcAppDrawContext *draw_ctx);

    // Basic draw functions.
    void (*line)(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke);
    void (*polyline)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
    void (*polygon)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
    void (*convex_polygon_filled)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
    void (*rounded_polygon)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke);
    void (*rounded_convex_polygon_filled)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color);
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
    void (*convex_polygon_filled_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_polygon_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
    void (*rounded_convex_polygon_filled_ex)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
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
    DcAppDrawPlanetViewHandle (*planet_view_geodetic)(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, double lat, double lon, double elevation, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppPlanetViewOptions options, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
    DcAppDrawPlanetViewHandle (*planet_view_cartesian)(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, DcAppVec3d camera_position, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppPlanetViewOptions options, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
    bool (*planet_container_push_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppPlanetLocalTransform transform);
    void (*planet_container_pop)(DcAppDrawContext *draw_ctx);
    void (*planet_line_local)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float line_width, DcAppVec4 color);
    void (*planet_polygon_local)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float line_width, DcAppVec4 color);
    void (*planet_convex_polygon_filled_local)(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
    void (*planet_sphere_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, double radius, DcAppVec4 color);
    void (*planet_sphere_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3d position, float radius, DcAppVec4 color);
    void (*planet_line_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3d *points, uint32_t point_count, float line_width, DcAppVec4 color);
    void (*planet_line_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3d *points, uint32_t point_count, float line_width, DcAppVec4 color);
    void (*planet_polygon_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3d *points, uint32_t point_count, float line_width, DcAppVec4 color);
    void (*planet_polygon_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3d *points, uint32_t point_count, float line_width, DcAppVec4 color);
    void (*planet_convex_polygon_filled_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3d *points, uint32_t point_count, DcAppVec4 color);
    void (*planet_convex_polygon_filled_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3d *points, uint32_t point_count, DcAppVec4 color);
    void (*planet_ellipse_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppVec2 radius, float rotation_degrees, uint32_t segments, float line_width, DcAppVec4 color);
    void (*planet_ellipse_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3d center, DcAppVec2 radius, float rotation_degrees, uint32_t segments, float line_width, DcAppVec4 color);
    void (*planet_ellipse_filled_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppVec2 radius, float rotation_degrees, uint32_t segments, DcAppVec4 color);
    void (*planet_ellipse_filled_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3d center, DcAppVec2 radius, float rotation_degrees, uint32_t segments, DcAppVec4 color);
    void (*planet_image_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppTextureId texture_id, DcAppVec2 size, DcAppVec4 tint);
    void (*planet_image_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3d position, DcAppTextureId texture_id, DcAppVec2 size, DcAppVec4 tint);
    void (*planet_text_geodetic)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, const char *text, float size, DcAppVec4 color);
    void (*planet_text_cartesian)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3d position, const char *text, float size, DcAppVec4 color);
    void (*planet_geojson)(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppPlanetGeojsonHandle geojson, DcAppPlanetGeojsonStyle style);
};

struct DcAppMouseApi {
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
    // Captured press ended, possibly outside the target.
    bool (*released)(DcAppDrawContext *draw_ctx, const char *id);
    bool (*active)(DcAppDrawContext *draw_ctx, const char *id);
    // Captured press ended while the pointer was over the target.
    bool (*clicked)(DcAppDrawContext *draw_ctx, const char *id);

    // Current mouse state in the draw context's local space.
    bool (*down)(DcAppDrawContext *draw_ctx);
    const DcAppMouse *(*get_state)(DcAppDrawContext *draw_ctx);
};

#endif
