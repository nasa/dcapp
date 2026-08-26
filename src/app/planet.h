#ifndef DC_APP_PLANET_H
#define DC_APP_PLANET_H

#include "planet_api.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct _plPlanet plPlanet;
typedef struct _plPlanetView plPlanetView;
typedef struct DcAppPlanetContext DcAppPlanetContext;
typedef struct DcGeojson DcGeojson;
struct DcGeoCrsGeodetic;
struct DcGeoCrsCartesian;
struct DcGeoCrsPolarStereo;

//~ subsystem lifecycle

// stable subsystem-owned state with contents private to planet.c
void dc_app_planet_init(plApiRegistryI *api_registry);

DcAppPlanetContext *dc_app_planet_context_create(const char *asset_root);
void dc_app_planet_context_destroy(DcAppPlanetContext *planet_ctx);

//~ planet resources

DcAppPlanetHandle dc_app_planet_get_planet_by_id(DcAppPlanetContext *planet_ctx, const char *id);
DcAppPlanetHandle dc_app_planet_create_planet(DcAppPlanetContext *planet_ctx, DcAppPlanetCreateInfo info);
DcAppPlanetHandle dc_app_planet_create_planet_with_id(DcAppPlanetContext *planet_ctx, const char *id, DcAppPlanetCreateInfo info);
bool dc_app_planet_set_texture_geodetic(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel);
bool dc_app_planet_set_texture_cartesian(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, const char *path, DcAppVec3d position, float meters_per_pixel);
bool dc_app_planet_set_texture_geodetic_slot(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, double lat, double lon, float meters_per_pixel);
bool dc_app_planet_set_texture_cartesian_slot(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, DcAppVec3d position, float meters_per_pixel);
bool dc_app_planet_set_texture_projected_slot(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, double origin_x, double origin_y, float meters_per_pixel);
bool dc_app_planet_clear_texture(DcAppPlanetHandle planet, uint32_t slot);
bool dc_app_planet_set_light_direction(DcAppPlanetHandle planet, DcAppVec3 direction);

//~ views and overlays

DcAppPlanetViewHandle dc_app_planet_create_geodetic_view(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
DcAppPlanetViewHandle dc_app_planet_create_cartesian_view(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
bool dc_app_planet_set_view_shaders(DcAppPlanetViewHandle view, const char *vertex_shader, const char *fragment_shader);
DcAppPlanetGeojsonHandle dc_app_planet_load_geojson(DcAppPlanetContext *planet_ctx, const char *path);
DcAppPlanetBreadcrumbsHandle dc_app_planet_create_breadcrumbs(DcAppPlanetContext *planet_ctx, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing);
bool dc_app_planet_update_breadcrumbs_geodetic(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3d position);
bool dc_app_planet_update_breadcrumbs_cartesian(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppVec3d position);
void dc_app_planet_clear_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs);
DcAppPlanetBreadcrumbsPoints dc_app_planet_get_breadcrumbs_points(DcAppPlanetBreadcrumbsHandle breadcrumbs);

//~ registry metadata

uint32_t dc_app_planet_count(const DcAppPlanetContext *planet_ctx);
uint32_t dc_app_planet_view_count(const DcAppPlanetContext *planet_ctx);
DcAppPlanetHandle dc_app_planet_at(const DcAppPlanetContext *planet_ctx, uint32_t index);
DcAppPlanetViewHandle dc_app_planet_view_at(const DcAppPlanetContext *planet_ctx, uint32_t index);
uint8_t dc_app_planet_index(DcAppPlanetHandle planet);
uint8_t dc_app_planet_view_index(DcAppPlanetViewHandle view);
uint32_t dc_app_planet_view_width(DcAppPlanetViewHandle view);
uint32_t dc_app_planet_view_height(DcAppPlanetViewHandle view);
double dc_app_planet_radius(DcAppPlanetHandle planet);
const struct DcGeoCrsGeodetic *dc_app_planet_geodetic_crs(DcAppPlanetHandle planet);
const struct DcGeoCrsCartesian *dc_app_planet_cartesian_crs(DcAppPlanetHandle planet);
const struct DcGeoCrsPolarStereo *dc_app_planet_polar_crs(DcAppPlanetHandle planet);
bool dc_app_planet_uses_legacy_projected_origin(DcAppPlanetHandle planet);
DcGeojson *dc_app_planet_geojson(DcAppPlanetGeojsonHandle geojson);

//~ renderer integration

plPlanet *dc_app_planet_pl(DcAppPlanetHandle planet);
plPlanetView *dc_app_planet_view_pl(DcAppPlanetViewHandle view);
DcAppPlanetCrs dc_app_planet_view_crs(DcAppPlanetViewHandle view);
DcAppPlanetHandle dc_app_planet_view_planet(DcAppPlanetViewHandle view);

#endif
