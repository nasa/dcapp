#ifndef _DCAPP_PLANET_H_
#define _DCAPP_PLANET_H_

#include "dcapp.h"
#include "draw.h"
#include "geo.h"

// wraps a plPlanet created by the dcapp planet subsystem.
struct _DcAppPlanet {
    _AppData *app_data;
    plPlanet *planet;
    char *id;
    double radius;
    DcGeoCrsGeodetic geodetic_crs;
    DcGeoCrsCartesian cartesian_crs;
    DcGeoCrsPolarStereo polar_crs;
    bool legacy_projected_origin;
    uint8_t index;
};

// wraps a plPlanetView created by the dcapp planet subsystem.
struct _DcAppPlanetView {
    _AppData *app_data;
    DcAppPlanetHandle planet;
    plPlanetView *view;
    DcAppPlanetCrs crs;
    uint32_t width;
    uint32_t height;
    uint8_t index;
};

struct _DcAppPlanetBreadcrumbs {
    _AppData *app_data;
    DcAppPlanetCrs crs;
    uint32_t max_points;
    float point_spacing;
    DcAppVec3 *sb_points;
};

DcAppPlanetHandle dc_app_planet_get_planet_by_id(_AppData *app_data, const char *id);
DcAppPlanetHandle dc_app_planet_create_planet(_AppData *app_data, DcAppPlanetCreateInfo info);
DcAppPlanetHandle dc_app_planet_create_planet_with_id(_AppData *app_data, const char *id, DcAppPlanetCreateInfo info);
bool dc_app_planet_set_texture_geodetic(_AppData *app_data, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel);
bool dc_app_planet_set_texture_cartesian(_AppData *app_data, DcAppPlanetHandle planet, const char *path, DcAppVec3 position, float meters_per_pixel);
DcAppPlanetViewHandle dc_app_planet_create_geodetic_view(_AppData *app_data, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
DcAppPlanetViewHandle dc_app_planet_create_cartesian_view(_AppData *app_data, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
bool dc_app_planet_set_view_shaders(DcAppPlanetViewHandle view, const char *vertex_shader, const char *fragment_shader);
DcAppPlanetBreadcrumbsHandle dc_app_planet_create_breadcrumbs(_AppData *app_data, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing);
void dc_app_planet_update_breadcrumbs_geodetic(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3 position);
void dc_app_planet_update_breadcrumbs_cartesian(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppVec3 position);
void dc_app_planet_clear_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs);
DcAppPlanetBreadcrumbsPoints dc_app_planet_get_breadcrumbs_points(DcAppPlanetBreadcrumbsHandle breadcrumbs);

void dc_app_planet_free_wrappers(_AppData *app_data);

// unwraps handles for internal draw and xml integration.
plPlanet *dc_app_planet_pl(DcAppPlanetHandle planet);
plPlanetView *dc_app_planet_view_pl(DcAppPlanetViewHandle view);
DcAppPlanetCrs dc_app_planet_view_crs(DcAppPlanetViewHandle view);
DcAppPlanetHandle dc_app_planet_view_planet(DcAppPlanetViewHandle view);

const DcAppPlanetApi *dc_app_planet_api(void);

#endif
