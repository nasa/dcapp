#ifndef DC_APP_DRAW_INTERNAL_H
#define DC_APP_DRAW_INTERNAL_H

#include "draw_api.h"

typedef uint64_t DcAppDrawTargetId;

// Node drawing uses screen-space input because it resolves its own transforms.
const DcAppMouse *dc_app_draw_context_get_screen_mouse(DcAppDrawContext *draw_ctx);

// Internal targets share interaction state with logic-registered string IDs,
// while remaining in a separate identity namespace.
void dc_app_draw_mouse_register_target(DcAppDrawContext *draw_ctx, DcAppDrawTargetId target_id);
bool dc_app_draw_mouse_target_hovered(DcAppDrawContext *draw_ctx, DcAppDrawTargetId target_id);
bool dc_app_draw_mouse_target_pressed(DcAppDrawContext *draw_ctx, DcAppDrawTargetId target_id);
bool dc_app_draw_mouse_target_released(DcAppDrawContext *draw_ctx, DcAppDrawTargetId target_id);
bool dc_app_draw_mouse_target_active(DcAppDrawContext *draw_ctx, DcAppDrawTargetId target_id);

// XML styles track line/fill presence independently from color alpha.
void dc_app_draw_planet_polygon_local_enabled(
    DcAppDrawContext *draw_ctx,
    const DcAppVec2 *points,
    uint32_t point_count,
    float line_width,
    uint32_t line_color,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled);
void dc_app_draw_planet_polygon_cartesian_enabled(
    DcAppDrawPlanetViewHandle view,
    const DcAppVec3 *points,
    uint32_t point_count,
    float line_width,
    uint32_t line_color,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled);
void dc_app_draw_planet_ellipse_cartesian_enabled(
    DcAppDrawPlanetViewHandle view,
    const DcAppVec3 *center,
    const DcAppVec2 *radius,
    float rotation_degrees,
    uint32_t segments,
    float line_width,
    uint32_t line_color,
    bool line_enabled,
    uint32_t fill_color,
    bool fill_enabled);

#endif
