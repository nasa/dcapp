#ifndef DC_APP_DRAW_H
#define DC_APP_DRAW_H

#include "draw_api.h"

#include "pl.h"
#include "pl_math.h"
#include "dc_draw_ext.h"

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct _plCamera plCamera;
typedef struct _plPlanetView plPlanetView;
typedef struct _plRenderEncoder plRenderEncoder;

struct DcAppTextureContext;

#ifndef DCAPP_LINE_WIDTH_FACTOR
#define DCAPP_LINE_WIDTH_FACTOR 1.2f
#endif

#ifndef DCAPP_DRAW_POINT_COUNT_MAX
#define DCAPP_DRAW_POINT_COUNT_MAX 65536
#endif

typedef struct DcAppDrawScope {
    DcAppDrawArea area;
    int container_count;
    int stencil_count;
    int planet_view_count;
    int planet_container_count;
} DcAppDrawScope;

typedef struct DcAppDrawFrameInput {
    plVec2 mouse_position;
    bool mouse_position_valid;
    bool mouse_down;
} DcAppDrawFrameInput;

// draw module and frame context
void dc_app_draw_init(plApiRegistryI *api_registry);
DcAppDrawContext *dc_app_draw_context_create(dcFont *default_font, struct DcAppTextureContext *texture_ctx);
void dc_app_draw_context_destroy(DcAppDrawContext *draw_ctx);
// Begins a frame from raw input; pressed/released edges are derived internally.
void dc_app_draw_context_begin(DcAppDrawContext *draw_ctx, DcAppDrawFrameInput input);
void dc_app_draw_context_end(DcAppDrawContext *draw_ctx);
void dc_app_draw_context_submit(DcAppDrawContext *draw_ctx, plRenderEncoder *encoder);
// Publishes targets registered during the frame for the next draw pass.
void dc_app_draw_context_commit(DcAppDrawContext *draw_ctx);
// Renderer nodes install resolved frames here; logic callbacks use container helpers below.
void dc_app_draw_context_push(DcAppDrawContext *draw_ctx, plVec2 position, plVec2 dimensions, const plMat4 *transform);
void dc_app_draw_context_pop(DcAppDrawContext *draw_ctx);
DcAppDrawScope dc_app_draw_scope_begin(DcAppDrawContext *draw_ctx);
void dc_app_draw_scope_end(DcAppDrawContext *draw_ctx, DcAppDrawScope scope);

// DrawFunction primitive API
const DcAppDrawArea *dc_app_draw_get_area(DcAppDrawContext *draw_ctx);
void dc_app_draw_line(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke);
void dc_app_draw_line_ex(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polyline(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
void dc_app_draw_polyline_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_triangles_filled_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polygon(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
void dc_app_draw_polygon_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_convex_polygon_filled(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
void dc_app_draw_convex_polygon_filled_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_polygon(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_polygon_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_convex_polygon_filled(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_convex_polygon_filled_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_quad(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke);
void dc_app_draw_quad_ex(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_quad_filled(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color);
void dc_app_draw_quad_filled_ex(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_quad(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_quad_ex(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_quad_filled(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_quad_filled_ex(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_image(DcAppDrawContext *draw_ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint);
void dc_app_draw_image_ex(DcAppDrawContext *draw_ctx, DcAppTextureId texture_id, DcAppVec2 position, DcAppVec2 size, DcAppVec4 tint, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_image_quad(DcAppDrawContext *draw_ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 position, DcAppPlacement placement, DcAppDrawArea *out_area);
void dc_app_draw_image_quad_uv(DcAppDrawContext *draw_ctx, uint32_t texture_id, DcAppVec2 p0, DcAppVec2 p1, DcAppVec2 p2, DcAppVec2 p3, DcAppVec2 uv0, DcAppVec2 uv1, DcAppVec2 uv2, DcAppVec2 uv3, DcAppVec2 position, DcAppPlacement placement, DcAppVec4 tint, DcAppDrawArea *out_area);
void dc_app_draw_rect(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke);
void dc_app_draw_rect_ex(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rect_filled(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color);
void dc_app_draw_rect_filled_ex(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_rect(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_rect_ex(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_rect_filled(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_rect_filled_ex(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, float corner_radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_circle(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppStroke stroke);
void dc_app_draw_circle_ex(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_circle_filled(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppVec4 color);
void dc_app_draw_circle_filled_ex(DcAppDrawContext *draw_ctx, DcAppVec2 center, float radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_ellipse(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppStroke stroke);
void dc_app_draw_ellipse_ex(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppStroke stroke, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_ellipse_filled(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppVec4 color);
void dc_app_draw_ellipse_filled_ex(DcAppDrawContext *draw_ctx, DcAppVec2 center, DcAppVec2 radius, DcAppVec4 color, DcAppPlacement placement, DcAppDrawResult *result);
DcAppVec2 dc_app_draw_text_size(DcAppDrawContext *draw_ctx, const char *text, DcAppTextStyle style);
void dc_app_draw_text(DcAppDrawContext *draw_ctx, DcAppVec2 position, const char *text, DcAppTextStyle style);
void dc_app_draw_text_ex(DcAppDrawContext *draw_ctx, DcAppVec2 position, const char *text, DcAppTextStyle style, DcAppPlacement placement, DcAppDrawResult *result);

// DrawFunction utility API
void dc_app_draw_resolve_points(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement, plVec2 *out, DcAppDrawArea *out_area);
bool dc_app_draw_container_push(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size);
bool dc_app_draw_container_push_ex(DcAppDrawContext *draw_ctx, DcAppVec2 position, DcAppVec2 size, DcAppVec2 virtual_size, DcAppPlacement placement, DcAppDrawResult *result);
bool dc_app_draw_container_push_area(DcAppDrawContext *draw_ctx, const DcAppDrawArea *area);
void dc_app_draw_container_pop(DcAppDrawContext *draw_ctx);
bool dc_app_draw_stencil_begin(DcAppDrawContext *draw_ctx);
void dc_app_draw_stencil_add(DcAppDrawContext *draw_ctx);
void dc_app_draw_stencil_remove(DcAppDrawContext *draw_ctx);
void dc_app_draw_stencil_draw(DcAppDrawContext *draw_ctx);
void dc_app_draw_stencil_end(DcAppDrawContext *draw_ctx);
DcAppDrawPlanetViewHandle dc_app_draw_planet_view_geodetic(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, double lat, double lon, double elevation, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppPlanetViewOptions options, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
DcAppDrawPlanetViewHandle dc_app_draw_planet_view_cartesian(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, DcAppVec3 camera_position, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppPlanetViewOptions options, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
bool dc_app_draw_planet_container_push_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppPlanetLocalTransform transform);
void dc_app_draw_planet_container_pop(DcAppDrawContext *draw_ctx);
void dc_app_draw_planet_line_local(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float line_width, DcAppVec4 color);
void dc_app_draw_planet_polygon_local(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_sphere_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, double radius, DcAppVec4 color);
void dc_app_draw_planet_sphere_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, float radius, DcAppVec4 color);
void dc_app_draw_planet_line_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 color);
void dc_app_draw_planet_line_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 color);
void dc_app_draw_planet_polygon_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_polygon_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_ellipse_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppVec2 radius, float rotation_degrees, uint32_t segments, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_ellipse_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 center, DcAppVec2 radius, float rotation_degrees, uint32_t segments, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_image_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, DcAppTextureId texture_id, DcAppVec2 size, DcAppVec4 tint);
void dc_app_draw_planet_image_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, DcAppTextureId texture_id, DcAppVec2 size, DcAppVec4 tint);
void dc_app_draw_planet_text_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, const char *text, float size, DcAppVec4 color);
void dc_app_draw_planet_text_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, const char *text, float size, DcAppVec4 color);
void dc_app_draw_planet_geojson(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppPlanetGeojsonHandle geojson, DcAppPlanetGeojsonStyle style);

// DrawFunction mouse registration and event API
void dc_app_mouse_rect(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 position, DcAppVec2 size);
void dc_app_mouse_rect_ex(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement);
void dc_app_mouse_circle(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, float radius);
void dc_app_mouse_circle_ex(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, float radius, DcAppPlacement placement);
void dc_app_mouse_ellipse(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, DcAppVec2 radius);
void dc_app_mouse_ellipse_ex(DcAppDrawContext *draw_ctx, const char *id, DcAppVec2 center, DcAppVec2 radius, DcAppPlacement placement);
void dc_app_mouse_polygon(DcAppDrawContext *draw_ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position);
void dc_app_mouse_polygon_ex(DcAppDrawContext *draw_ctx, const char *id, const DcAppVec2 *points, uint32_t point_count, DcAppVec2 position, DcAppPlacement placement);
bool dc_app_mouse_hovered(DcAppDrawContext *draw_ctx, const char *id);
bool dc_app_mouse_pressed(DcAppDrawContext *draw_ctx, const char *id);
bool dc_app_mouse_released(DcAppDrawContext *draw_ctx, const char *id);
bool dc_app_mouse_active(DcAppDrawContext *draw_ctx, const char *id);
bool dc_app_mouse_clicked(DcAppDrawContext *draw_ctx, const char *id);
bool dc_app_mouse_down(DcAppDrawContext *draw_ctx);
const DcAppMouse *dc_app_mouse_get_state(DcAppDrawContext *draw_ctx);

// Internal XML/node draw helpers not exposed through DrawFunction yet
plVec2 dc_app_draw_text_options_size(const char *text, dcDrawTextOptions options);
void   dc_app_draw_text_options(DcAppDrawContext *draw_ctx, const char *text, dcDrawTextOptions options);
void   dc_app_draw_3d_sphere_textured(DcAppDrawContext *draw_ctx, uint32_t texture_id, plSphere sphere, const plMat4 *transform, uint32_t color);
void   dc_app_draw_3d_sphere_filled(DcAppDrawContext *draw_ctx, plSphere sphere, uint32_t color);
void   dc_app_draw_planet_polygon_filled(plPlanetView *view, plVec3 *points, uint32_t point_count, uint32_t color);
void   dc_app_draw_planet_polygon(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color);
void   dc_app_draw_planet_line(plPlanetView *view, plVec3 *points, uint32_t point_count, float line_width, uint32_t color);
void   dc_app_draw_planet_ellipse(plPlanetView *view, plVec3 center, plVec2 radius, float rotation_degrees, uint32_t segments, float line_width, uint32_t line_color, bool line_enabled, uint32_t fill_color, bool fill_enabled);
void   dc_app_draw_planet_sphere(plPlanetView *view, float lon, float lat, float height, float radius, uint32_t color);
void   dc_app_draw_planet_text(plPlanetView *view, plCamera *camera, plVec3 position, const char *text, float size, uint32_t color);

const DcAppDrawApi *dc_app_draw_api(void);
const DcAppMouseApi *dc_app_mouse_api(void);

#endif
