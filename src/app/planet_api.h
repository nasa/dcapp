#ifndef DC_APP_PLANET_API_H
#define DC_APP_PLANET_API_H

#include "app/planet_types.h"
#include "app/vector.h"

#include <stdbool.h>
#include <stdint.h>

#define DC_APP_PLANET_TEXTURE_SLOT_COUNT 5u
#define DC_APP_PLANET_ELLIPSE_MAX_SEGMENTS 1000u

typedef struct DcAppContext DcAppContext;
typedef struct DcAppPlanet *DcAppPlanetHandle;
typedef struct DcAppPlanetView *DcAppPlanetViewHandle;
typedef struct DcAppPlanetBreadcrumbs *DcAppPlanetBreadcrumbsHandle;
typedef struct DcAppPlanetGeojson *DcAppPlanetGeojsonHandle;
typedef struct DcAppPlanetLocalTransform DcAppPlanetLocalTransform;
typedef struct DcAppPlanetBreadcrumbsPoints DcAppPlanetBreadcrumbsPoints;
typedef struct DcAppPlanetViewOptions DcAppPlanetViewOptions;
typedef struct DcAppPlanetGeojsonStyle DcAppPlanetGeojsonStyle;
typedef struct DcAppPlanetCreateInfo DcAppPlanetCreateInfo;
typedef struct DcAppPlanetApi DcAppPlanetApi;

// Local geometry starts in tangent-plane units before scale and rotation.
// Overlay line widths remain in logical display pixels.
struct DcAppPlanetLocalTransform {
    float scale;
    float rotation_degrees;
};

// The points are borrowed until the breadcrumbs are next mutated or destroyed.
struct DcAppPlanetBreadcrumbsPoints {
    const DcAppVec3d *points;
    uint32_t count;
    DcAppPlanetCrs crs;
};

struct DcAppPlanetViewOptions {
    int flags;
    float tau;
};

struct DcAppPlanetGeojsonStyle {
    int flags;
    double height_above_terrain;
    float line_width;
    DcAppVec4 line_color;
    DcAppVec4 fill_color;
};

struct DcAppPlanetCreateInfo {
    const char *data_path;
    uint32_t mesh_cache_size_mb; // combined cache size in MiB, 0 = renderer default
};

struct DcAppPlanetApi {
    // planet resources live until app shutdown.
    DcAppPlanetHandle (*get_planet_by_id)(DcAppContext *app_ctx, const char *id);
    DcAppPlanetHandle (*create_planet)(DcAppContext *app_ctx, DcAppPlanetCreateInfo info);
    // fails if id already exists.
    DcAppPlanetHandle (*create_planet_with_id)(DcAppContext *app_ctx, const char *id, DcAppPlanetCreateInfo info);
    bool (*set_texture_geodetic)(DcAppContext *app_ctx, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel);
    bool (*set_texture_cartesian)(DcAppContext *app_ctx, DcAppPlanetHandle planet, const char *path, DcAppVec3d position, float meters_per_pixel);
    bool (*set_texture_projected)(DcAppContext *app_ctx, DcAppPlanetHandle planet, const char *path, double origin_x, double origin_y, float meters_per_pixel);
    bool (*set_texture_geodetic_slot)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, double lat, double lon, float meters_per_pixel);
    bool (*set_texture_cartesian_slot)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, DcAppVec3d position, float meters_per_pixel);
    bool (*set_texture_projected_slot)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, double origin_x, double origin_y, float meters_per_pixel);
    bool (*clear_texture)(DcAppPlanetHandle planet, uint32_t slot);
    bool (*set_light_direction)(DcAppPlanetHandle planet, DcAppVec3 direction);
    DcAppPlanetViewHandle (*create_geodetic_view)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
    DcAppPlanetViewHandle (*create_cartesian_view)(DcAppContext *app_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
    bool (*set_view_shaders)(DcAppPlanetViewHandle view, const char *vertex_shader, const char *fragment_shader);
    DcAppPlanetGeojsonHandle (*load_geojson)(DcAppContext *app_ctx, const char *path);
    DcAppPlanetBreadcrumbsHandle (*create_breadcrumbs)(DcAppContext *app_ctx, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing);
    // returns true only when the position is appended.
    bool (*update_breadcrumbs_geodetic)(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3d position);
    bool (*update_breadcrumbs_cartesian)(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppVec3d position);
    void (*clear_breadcrumbs)(DcAppPlanetBreadcrumbsHandle breadcrumbs);
    DcAppPlanetBreadcrumbsPoints (*get_breadcrumbs_points)(DcAppPlanetBreadcrumbsHandle breadcrumbs);
};

#endif
