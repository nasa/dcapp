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

typedef struct _DcAppStencilHandler {
    int depth;
} DcAppStencilHandler;

// draw batch utils
void           dc_app_draw_batch_reset(_AppData *app_data);
dcDrawLayer2D *dc_app_draw_batch_get_2d(_AppData *app_data);
dcDrawList3D  *dc_app_draw_batch_get_3d(_AppData *app_data);

// DrawFunction context and placement helpers
DcAppDrawContext dc_app_draw_context(_AppData *app_data, _NodeIndex node_index, plVec2 parent_position, plVec2 parent_dimensions, const plMat4 *parent_transform);
void             dc_app_draw_context_cleanup(DcAppDrawContext *draw_ctx);

// DrawFunction primitive API
const DcAppDrawArea *dc_app_draw_get_area(DcAppDrawContext *draw_ctx);
void dc_app_draw_line(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke);
void dc_app_draw_line_ex(DcAppDrawContext *draw_ctx, DcAppVec2 p0, DcAppVec2 p1, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polyline(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
void dc_app_draw_polyline_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_triangles_filled_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polygon(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke);
void dc_app_draw_polygon_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_polygon_filled(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color);
void dc_app_draw_polygon_filled_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_polygon(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke);
void dc_app_draw_rounded_polygon_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppStroke stroke, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_rounded_polygon_filled(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color);
void dc_app_draw_rounded_polygon_filled_ex(DcAppDrawContext *draw_ctx, const DcAppVec2 *points, uint32_t point_count, float corner_radius, DcAppVec4 color, DcAppVec2 position, DcAppPlacement placement, DcAppDrawResult *result);
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
DcAppDrawPlanetViewHandle dc_app_draw_planet_view_geodetic(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, double lat, double lon, double elevation, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
DcAppDrawPlanetViewHandle dc_app_draw_planet_view_cartesian(DcAppDrawContext *draw_ctx, DcAppPlanetViewHandle view, DcAppVec3 camera_position, DcAppVec3 rpy, float fov_degrees, bool orthographic, DcAppVec2 position, DcAppVec2 size, DcAppPlacement placement, DcAppDrawResult *result);
void dc_app_draw_planet_sphere_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, double radius, DcAppVec4 color);
void dc_app_draw_planet_sphere_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, float radius, DcAppVec4 color);
void dc_app_draw_planet_line_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 color);
void dc_app_draw_planet_line_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 color);
void dc_app_draw_planet_polygon_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_polygon_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, const DcAppVec3 *points, uint32_t point_count, float line_width, DcAppVec4 line_color, DcAppVec4 fill_color);
void dc_app_draw_planet_text_geodetic(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, double lat, double lon, double height, const char *text, float size, DcAppVec4 color);
void dc_app_draw_planet_text_cartesian(DcAppDrawContext *draw_ctx, DcAppDrawPlanetViewHandle view, DcAppVec3 position, const char *text, float size, DcAppVec4 color);

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
const DcAppTextureApi *dc_app_texture_api(void);
const DcAppPlanetApi *dc_app_planet_api(void);

#endif
