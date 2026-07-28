#include "planet.h"

#define PL_EXPERIMENTAL
#include "pl.h"
#include "pl_json.h"
#include "pl_planet_ext.h"
#include "pl_planet_processor_ext.h"
#include "pl_starter_ext.h"
#include "pl_vfs_ext.h"
#include "geo.h"
#include "geojson.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/stb_sb.h"
#include "utils/string.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const plMemoryI  *_ext_memory  = NULL;
static const plStarterI *_ext_starter = NULL;
static const plPlanetI  *_ext_planet  = NULL;
static const plVfsI     *_ext_vfs     = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

// Runtime handles are separately allocated so registry growth cannot invalidate them.
struct DcAppPlanetContext {
    char *asset_root;
    bool extension_initialized;

    plPlanet **sb_planets;
    plPlanetView **sb_views;
    DcAppPlanetHandle *sb_planet_handles;
    DcAppPlanetViewHandle *sb_view_handles;
    DcAppPlanetBreadcrumbsHandle *sb_breadcrumbs;
    DcAppPlanetGeojsonHandle *sb_geojsons;
};

struct DcAppPlanet {
    plPlanet *planet;
    char *id;
    double radius;
    DcGeoCrsGeodetic geodetic_crs;
    DcGeoCrsCartesian cartesian_crs;
    DcGeoCrsPolarStereo polar_crs;
    bool legacy_projected_origin;
    uint8_t index;
};

struct DcAppPlanetView {
    DcAppPlanetContext *owner;
    DcAppPlanetHandle planet;
    plPlanetView *view;
    DcAppPlanetCrs crs;
    uint32_t width;
    uint32_t height;
    uint8_t index;
    char *vertex_shader_path;
    char *fragment_shader_path;
};

struct DcAppPlanetBreadcrumbs {
    DcAppPlanetCrs crs;
    uint32_t max_points;
    float point_spacing;
    DcAppVec3d *sb_points;
};

struct DcAppPlanetGeojson {
    DcGeojson *geojson;
};

static void _planet_ensure_initialized(DcAppPlanetContext *planet_ctx);
static bool _planet_load_process_info(const char *json_path, double *out_radius, plPlanetProcessInfo *out_info, bool *out_legacy_projected_origin);
static void _planet_free_process_info(plPlanetProcessInfo *info);
static bool _planet_file_path_to_vfs(DcAppPlanetContext *planet_ctx, const char *path, char *out, size_t out_size);
static bool _planet_file_path_to_absolute(DcAppPlanetContext *planet_ctx, const char *path, char *out, size_t out_size);
static DcAppPlanetViewHandle _planet_create_view(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, DcAppPlanetCrs crs, uint32_t width, uint32_t height);
static bool _planet_update_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3d position);
static double _planet_breadcrumbs_distance(DcAppPlanetHandle planet, DcAppPlanetCrs crs, DcAppVec3d a, DcAppVec3d b);

void dc_app_planet_init(plApiRegistryI *api_registry) {
    _ext_memory  = pl_get_api_latest(api_registry, plMemoryI);
    _ext_starter = pl_get_api_latest(api_registry, plStarterI);
    _ext_planet  = pl_get_api_latest(api_registry, plPlanetI);
    _ext_vfs     = pl_get_api_latest(api_registry, plVfsI);
}

DcAppPlanetContext *dc_app_planet_context_create(const char *asset_root) {
    DcAppPlanetContext *planet_ctx = (DcAppPlanetContext *)PL_ALLOC(sizeof(*planet_ctx));
    if (!planet_ctx) return NULL;
    memset(planet_ctx, 0, sizeof(*planet_ctx));

    if (asset_root && asset_root[0] != '\0') {
        size_t length = strlen(asset_root) + 1;
        planet_ctx->asset_root = (char *)PL_ALLOC(length);
        if (!planet_ctx->asset_root) {
            PL_FREE(planet_ctx);
            return NULL;
        }
        memcpy(planet_ctx->asset_root, asset_root, length);
    }

    // Resource indices are one-based; index zero is always undefined.
    sbpush(planet_ctx->sb_planets, NULL);
    sbpush(planet_ctx->sb_views, NULL);
    return planet_ctx;
}

void dc_app_planet_context_destroy(DcAppPlanetContext *planet_ctx) {
    if (!planet_ctx) return;

    for (int i = 0; i < sbcount(planet_ctx->sb_geojsons); i++) {
        DcAppPlanetGeojsonHandle geojson = planet_ctx->sb_geojsons[i];
        if (!geojson) continue;
        dc_geojson_free(geojson->geojson);
        PL_FREE(geojson);
    }
    sbfree(planet_ctx->sb_geojsons);

    for (int i = 0; i < sbcount(planet_ctx->sb_breadcrumbs); i++) {
        DcAppPlanetBreadcrumbsHandle breadcrumbs = planet_ctx->sb_breadcrumbs[i];
        if (!breadcrumbs) continue;
        sbfree(breadcrumbs->sb_points);
        PL_FREE(breadcrumbs);
    }
    sbfree(planet_ctx->sb_breadcrumbs);

    // cleans up every planet view created by the shared planet subsystem.
    for (int i = 0; i < sbcount(planet_ctx->sb_view_handles); i++) {
        DcAppPlanetViewHandle view = planet_ctx->sb_view_handles[i];
        if (!view) continue;
        if (view->view) _ext_planet->cleanup_view(view->view);
        if (view->vertex_shader_path) PL_FREE(view->vertex_shader_path);
        if (view->fragment_shader_path) PL_FREE(view->fragment_shader_path);
        PL_FREE(view);
    }
    sbfree(planet_ctx->sb_view_handles);

    // cleans up every planet created by the shared planet subsystem.
    for (int i = 0; i < sbcount(planet_ctx->sb_planet_handles); i++) {
        DcAppPlanetHandle planet = planet_ctx->sb_planet_handles[i];
        if (!planet) continue;
        if (planet->planet) _ext_planet->cleanup_planet(planet->planet);
        if (planet->id) PL_FREE(planet->id);
        PL_FREE(planet);
    }
    sbfree(planet_ctx->sb_planet_handles);
    sbfree(planet_ctx->sb_views);
    sbfree(planet_ctx->sb_planets);

    if (planet_ctx->extension_initialized) _ext_planet->cleanup();

    if (planet_ctx->asset_root) PL_FREE(planet_ctx->asset_root);
    PL_FREE(planet_ctx);
}

DcAppPlanetHandle dc_app_planet_get_planet_by_id(DcAppPlanetContext *planet_ctx, const char *id) {
    if (!planet_ctx || !id || id[0] == '\0') return NULL;

    for (int i = 0; i < sbcount(planet_ctx->sb_planet_handles); i++) {
        DcAppPlanetHandle planet = planet_ctx->sb_planet_handles[i];
        if (planet && planet->id && strcmp(planet->id, id) == 0) return planet;
    }

    return NULL;
}

DcAppPlanetHandle dc_app_planet_create_planet(DcAppPlanetContext *planet_ctx, DcAppPlanetCreateInfo info) {
    if (!planet_ctx || !info.data_path || info.data_path[0] == '\0') return NULL;

    // initializes the planet extension on first use.
    _planet_ensure_initialized(planet_ctx);
    if (sbcount(planet_ctx->sb_planets) > UINT8_MAX) {
        DC_LOG_ERROR("Planet", "Too many planets; dcapp supports at most %u planet handles", UINT8_MAX);
        return NULL;
    }

    double radius = 0.0;
    plPlanetProcessInfo process_info = {0};
    bool legacy_projected_origin = false;
    if (!_planet_load_process_info(info.data_path, &radius, &process_info, &legacy_projected_origin)) {
        _planet_free_process_info(&process_info);
        return NULL;
    }
    plProjectionParams projection = process_info.tProjection;

    plPlanetInit planet_init = {0};
    planet_init.dRadius = radius;

    uint32_t mesh_cache_size_mb = info.mesh_cache_size_mb;
    if (mesh_cache_size_mb > UINT32_MAX / (1024u * 1024u / 2u)) {
        DC_LOG_WARN("Planet", "mesh_cache_size_mb is %u MiB; using renderer default instead", mesh_cache_size_mb);
        mesh_cache_size_mb = 0;
    }
    if (mesh_cache_size_mb > 0) {
        uint32_t buffer_size = mesh_cache_size_mb * (1024u * 1024u / 2u);
        planet_init.uVertexBufferSize = buffer_size;
        planet_init.uIndexBufferSize  = buffer_size;
    }

    // delegates renderer and streaming allocation to pl_planet_ext.
    plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
    plPlanet *planet = _ext_planet->create_planet(cmd_buf, planet_init, &process_info);
    if (planet) _ext_planet->prepare(planet, cmd_buf);
    _ext_starter->submit_temporary_command_buffer(cmd_buf);
    _planet_free_process_info(&process_info);
    if (!planet) return NULL;

    DcAppPlanetHandle handle = (DcAppPlanetHandle)PL_ALLOC(sizeof(*handle));
    memset(handle, 0, sizeof(*handle));
    handle->planet = planet;
    handle->radius = radius;
    // stores crs helpers so xml and logic share the same conversions.
    handle->geodetic_crs = dc_geo_create_crs_geodetic(radius);
    handle->cartesian_crs = dc_geo_create_crs_cartesian(radius);
    handle->polar_crs = dc_geo_create_crs_polar_stereographic(radius,
        projection.tPolarStereo.dLatitudeOfOrigin,
        projection.tPolarStereo.dLongitudeOfOrigin);
    handle->polar_crs.scale_factor = projection.tPolarStereo.dScaleFactor;
    handle->polar_crs.false_easting = projection.tPolarStereo.dFalseEasting;
    handle->polar_crs.false_northing = projection.tPolarStereo.dFalseNorthing;
    handle->legacy_projected_origin = legacy_projected_origin;
    sbpush(planet_ctx->sb_planets, planet);
    handle->index = (uint8_t)(sbcount(planet_ctx->sb_planets) - 1);
    sbpush(planet_ctx->sb_planet_handles, handle);
    return handle;
}

DcAppPlanetHandle dc_app_planet_create_planet_with_id(DcAppPlanetContext *planet_ctx, const char *id, DcAppPlanetCreateInfo info) {
    if (!planet_ctx || !id || id[0] == '\0') return NULL;
    if (dc_app_planet_get_planet_by_id(planet_ctx, id)) {
        DC_LOG_ERROR("Planet", "Planet id already exists: %s", id);
        return NULL;
    }

    DcAppPlanetHandle planet = dc_app_planet_create_planet(planet_ctx, info);
    if (planet) {
        size_t len = strlen(id) + 1;
        planet->id = (char *)PL_ALLOC(len);
        memcpy(planet->id, id, len);
    }
    return planet;
}

bool dc_app_planet_set_texture_geodetic(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel) {
    return dc_app_planet_set_texture_geodetic_slot(planet_ctx, planet, 0, path, lat, lon, meters_per_pixel);
}

bool dc_app_planet_set_texture_geodetic_slot(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, double lat, double lon, float meters_per_pixel) {
    if (!planet || !planet->planet || slot >= PL_PLANET_TEXTURE_SLOT_COUNT) return false;
    if (!planet_ctx || meters_per_pixel <= 0.0f) {
        _ext_planet->set_texture(planet->planet, NULL, slot);
        return false;
    }

    char vfs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (!_planet_file_path_to_vfs(planet_ctx, path, vfs_path, sizeof(vfs_path))) {
        _ext_planet->set_texture(planet->planet, NULL, slot);
        return false;
    }

    plVec3d geodetic_in = {lat, lon, 0.0};
    plVec2d polar_out;
    if (planet->legacy_projected_origin) {
        // Old planet metadata expects the historical user-longitude projection
        // convention. New metadata uses real projected CRS meters.
        dc_geo_user_geodetic_to_polar_stereo_d(&planet->geodetic_crs, &planet->polar_crs, &geodetic_in, &polar_out, 1);
        polar_out.y = -polar_out.y;
    } else {
        dc_geo_geodetic_to_polar_stereo_d(&planet->geodetic_crs, &planet->polar_crs, &geodetic_in, &polar_out, 1);
    }

    plPlanetTexture texture = {
        .pcPath = vfs_path,
        .fMetersPerPixel = meters_per_pixel,
        .dOriginX = polar_out.x,
        .dOriginY = polar_out.y,
    };
    return _ext_planet->set_texture(planet->planet, &texture, slot);
}

bool dc_app_planet_set_texture_cartesian(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, const char *path, DcAppVec3d position, float meters_per_pixel) {
    return dc_app_planet_set_texture_cartesian_slot(planet_ctx, planet, 0, path, position, meters_per_pixel);
}

bool dc_app_planet_set_texture_cartesian_slot(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, DcAppVec3d position, float meters_per_pixel) {
    if (!planet || !planet->planet || slot >= PL_PLANET_TEXTURE_SLOT_COUNT) return false;
    if (!planet_ctx || meters_per_pixel <= 0.0f) {
        _ext_planet->set_texture(planet->planet, NULL, slot);
        return false;
    }

    char vfs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (!_planet_file_path_to_vfs(planet_ctx, path, vfs_path, sizeof(vfs_path))) {
        _ext_planet->set_texture(planet->planet, NULL, slot);
        return false;
    }

    plVec3d cartesian_in = {position.x, position.y, position.z};
    plVec3d geodetic_out;
    plVec2d polar_out;
    dc_geo_cartesian_to_geodetic_d(&planet->cartesian_crs, &planet->geodetic_crs, &cartesian_in, &geodetic_out, 1);
    if (planet->legacy_projected_origin) {
        // Old planet metadata expects the historical user-longitude projection
        // convention. New metadata uses real projected CRS meters.
        dc_geo_user_geodetic_to_polar_stereo_d(&planet->geodetic_crs, &planet->polar_crs, &geodetic_out, &polar_out, 1);
        polar_out.y = -polar_out.y;
    } else {
        dc_geo_geodetic_to_polar_stereo_d(&planet->geodetic_crs, &planet->polar_crs, &geodetic_out, &polar_out, 1);
    }

    plPlanetTexture texture = {
        .pcPath = vfs_path,
        .fMetersPerPixel = meters_per_pixel,
        .dOriginX = polar_out.x,
        .dOriginY = polar_out.y,
    };
    return _ext_planet->set_texture(planet->planet, &texture, slot);
}

bool dc_app_planet_set_texture_projected_slot(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t slot, const char *path, double origin_x, double origin_y, float meters_per_pixel) {
    if (!planet || !planet->planet || slot >= PL_PLANET_TEXTURE_SLOT_COUNT) return false;
    if (!planet_ctx || meters_per_pixel <= 0.0f) {
        _ext_planet->set_texture(planet->planet, NULL, slot);
        return false;
    }

    char vfs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (!_planet_file_path_to_vfs(planet_ctx, path, vfs_path, sizeof(vfs_path))) {
        _ext_planet->set_texture(planet->planet, NULL, slot);
        return false;
    }

    plPlanetTexture texture = {
        .pcPath = vfs_path,
        .fMetersPerPixel = meters_per_pixel,
        .dOriginX = origin_x,
        .dOriginY = origin_y,
    };
    return _ext_planet->set_texture(planet->planet, &texture, slot);
}

bool dc_app_planet_clear_texture(DcAppPlanetHandle planet, uint32_t slot) {
    if (!planet || !planet->planet || slot >= PL_PLANET_TEXTURE_SLOT_COUNT) return false;
    return _ext_planet->set_texture(planet->planet, NULL, slot);
}

bool dc_app_planet_set_light_direction(DcAppPlanetHandle planet, DcAppVec3 direction) {
    if (!planet || !planet->planet) return false;
    plPlanetRuntimeOptions options = _ext_planet->get_runtime_options(planet->planet);
    options.tLightDirection = (plVec3){direction.x, direction.y, direction.z};
    _ext_planet->set_runtime_options(planet->planet, options);
    return true;
}

DcAppPlanetViewHandle dc_app_planet_create_geodetic_view(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height) {
    return _planet_create_view(planet_ctx, planet, DC_APP_PLANET_CRS_GEODETIC, width, height);
}

DcAppPlanetViewHandle dc_app_planet_create_cartesian_view(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, uint32_t width, uint32_t height) {
    return _planet_create_view(planet_ctx, planet, DC_APP_PLANET_CRS_CARTESIAN, width, height);
}

bool dc_app_planet_set_view_shaders(DcAppPlanetViewHandle view, const char *vertex_shader, const char *fragment_shader) {
    if (!view || !view->view) return false;

    char vertex_vfs[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    char fragment_vfs[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    const char *vertex_path = NULL;
    const char *fragment_path = NULL;

    if (vertex_shader && vertex_shader[0] != '\0') {
        if (!_planet_file_path_to_vfs(view->owner, vertex_shader, vertex_vfs, sizeof(vertex_vfs))) return false;
        vertex_path = vertex_vfs;
    }
    if (fragment_shader && fragment_shader[0] != '\0') {
        if (!_planet_file_path_to_vfs(view->owner, fragment_shader, fragment_vfs, sizeof(fragment_vfs))) return false;
        fragment_path = fragment_vfs;
    }

    // The view owns copies because the planet extension retains both path pointers.
    char *owned_vertex_path = NULL;
    char *owned_fragment_path = NULL;
    if (vertex_path) {
        const size_t size = strlen(vertex_path) + 1;
        owned_vertex_path = PL_ALLOC(size);
        if (!owned_vertex_path) return false;
        memcpy(owned_vertex_path, vertex_path, size);
    }
    if (fragment_path) {
        const size_t size = strlen(fragment_path) + 1;
        owned_fragment_path = PL_ALLOC(size);
        if (!owned_fragment_path) {
            if (owned_vertex_path) PL_FREE(owned_vertex_path);
            return false;
        }
        memcpy(owned_fragment_path, fragment_path, size);
    }

    _ext_planet->set_shaders(view->view, owned_vertex_path, owned_fragment_path);

    if (view->vertex_shader_path) PL_FREE(view->vertex_shader_path);
    if (view->fragment_shader_path) PL_FREE(view->fragment_shader_path);
    view->vertex_shader_path   = owned_vertex_path;
    view->fragment_shader_path = owned_fragment_path;
    return true;
}

DcAppPlanetBreadcrumbsHandle dc_app_planet_create_breadcrumbs(DcAppPlanetContext *planet_ctx, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing) {
    if (!planet_ctx) return NULL;
    if (crs != DC_APP_PLANET_CRS_GEODETIC && crs != DC_APP_PLANET_CRS_CARTESIAN) return NULL;
    if (max_points < 2) max_points = 2;
    if (point_spacing < 0.0f) point_spacing = 0.0f;

    DcAppPlanetBreadcrumbsHandle breadcrumbs = (DcAppPlanetBreadcrumbsHandle)PL_ALLOC(sizeof(*breadcrumbs));
    if (!breadcrumbs) return NULL;
    memset(breadcrumbs, 0, sizeof(*breadcrumbs));
    breadcrumbs->crs = crs;
    breadcrumbs->max_points = max_points;
    breadcrumbs->point_spacing = point_spacing;
    sbpush(planet_ctx->sb_breadcrumbs, breadcrumbs);
    return breadcrumbs;
}

bool dc_app_planet_update_breadcrumbs_geodetic(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3d position) {
    if (!breadcrumbs || breadcrumbs->crs != DC_APP_PLANET_CRS_GEODETIC || !planet) return false;
    return _planet_update_breadcrumbs(breadcrumbs, planet, position);
}

bool dc_app_planet_update_breadcrumbs_cartesian(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppVec3d position) {
    if (!breadcrumbs || breadcrumbs->crs != DC_APP_PLANET_CRS_CARTESIAN) return false;
    return _planet_update_breadcrumbs(breadcrumbs, NULL, position);
}

void dc_app_planet_clear_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs) {
    if (!breadcrumbs) return;
    sbclear(breadcrumbs->sb_points);
}

DcAppPlanetBreadcrumbsPoints dc_app_planet_get_breadcrumbs_points(DcAppPlanetBreadcrumbsHandle breadcrumbs) {
    if (!breadcrumbs) return (DcAppPlanetBreadcrumbsPoints){0};
    return (DcAppPlanetBreadcrumbsPoints){
        .points = breadcrumbs->sb_points,
        .count = (uint32_t)sbcount(breadcrumbs->sb_points),
        .crs = breadcrumbs->crs,
    };
}

DcAppPlanetGeojsonHandle dc_app_planet_load_geojson(DcAppPlanetContext *planet_ctx, const char *path) {
    char absolute_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (!_planet_file_path_to_absolute(planet_ctx, path, absolute_path, sizeof(absolute_path))) return NULL;

    DcGeojson *geojson = dc_geojson_load(absolute_path);
    if (!geojson) return NULL;
    if (dc_geojson_feature_count(geojson) == 0) {
        dc_geojson_free(geojson);
        return NULL;
    }

    DcAppPlanetGeojsonHandle handle = (DcAppPlanetGeojsonHandle)PL_ALLOC(sizeof(*handle));
    if (!handle) {
        dc_geojson_free(geojson);
        return NULL;
    }
    handle->geojson = geojson;
    sbpush(planet_ctx->sb_geojsons, handle);
    return handle;
}

uint32_t dc_app_planet_count(const DcAppPlanetContext *planet_ctx) {
    return planet_ctx ? (uint32_t)sbcount(planet_ctx->sb_planet_handles) : 0;
}

uint32_t dc_app_planet_view_count(const DcAppPlanetContext *planet_ctx) {
    return planet_ctx ? (uint32_t)sbcount(planet_ctx->sb_view_handles) : 0;
}

DcAppPlanetHandle dc_app_planet_at(const DcAppPlanetContext *planet_ctx, uint32_t index) {
    if (!planet_ctx || index >= (uint32_t)sbcount(planet_ctx->sb_planet_handles)) return NULL;
    return planet_ctx->sb_planet_handles[index];
}

DcAppPlanetViewHandle dc_app_planet_view_at(const DcAppPlanetContext *planet_ctx, uint32_t index) {
    if (!planet_ctx || index >= (uint32_t)sbcount(planet_ctx->sb_view_handles)) return NULL;
    return planet_ctx->sb_view_handles[index];
}

uint8_t dc_app_planet_index(DcAppPlanetHandle planet) {
    return planet ? planet->index : 0;
}

uint8_t dc_app_planet_view_index(DcAppPlanetViewHandle view) {
    return view ? view->index : 0;
}

uint32_t dc_app_planet_view_width(DcAppPlanetViewHandle view) {
    return view ? view->width : 0;
}

uint32_t dc_app_planet_view_height(DcAppPlanetViewHandle view) {
    return view ? view->height : 0;
}

double dc_app_planet_radius(DcAppPlanetHandle planet) {
    return planet ? planet->radius : 0.0;
}

const DcGeoCrsGeodetic *dc_app_planet_geodetic_crs(DcAppPlanetHandle planet) {
    return planet ? &planet->geodetic_crs : NULL;
}

const DcGeoCrsCartesian *dc_app_planet_cartesian_crs(DcAppPlanetHandle planet) {
    return planet ? &planet->cartesian_crs : NULL;
}

const DcGeoCrsPolarStereo *dc_app_planet_polar_crs(DcAppPlanetHandle planet) {
    return planet ? &planet->polar_crs : NULL;
}

bool dc_app_planet_uses_legacy_projected_origin(DcAppPlanetHandle planet) {
    return planet ? planet->legacy_projected_origin : false;
}

DcGeojson *dc_app_planet_geojson(DcAppPlanetGeojsonHandle geojson) {
    return geojson ? geojson->geojson : NULL;
}

plPlanet *dc_app_planet_pl(DcAppPlanetHandle planet) {
    return planet ? planet->planet : NULL;
}

plPlanetView *dc_app_planet_view_pl(DcAppPlanetViewHandle view) {
    return view ? view->view : NULL;
}

DcAppPlanetCrs dc_app_planet_view_crs(DcAppPlanetViewHandle view) {
    return view ? view->crs : DC_APP_PLANET_CRS_UNDEFINED;
}

DcAppPlanetHandle dc_app_planet_view_planet(DcAppPlanetViewHandle view) {
    return view ? view->planet : NULL;
}

static bool _planet_update_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3d position) {
    if (!isfinite(position.x) || !isfinite(position.y) || !isfinite(position.z)) return false;

    int point_count = sbcount(breadcrumbs->sb_points);
    if (point_count > 0) {
        double distance = _planet_breadcrumbs_distance(
            planet, breadcrumbs->crs, breadcrumbs->sb_points[point_count - 1], position);
        if (!(distance >= breadcrumbs->point_spacing)) return false;
    }

    sbpush(breadcrumbs->sb_points, position);

    point_count = sbcount(breadcrumbs->sb_points);
    if (point_count > (int)breadcrumbs->max_points) {
        sbshiftn(breadcrumbs->sb_points, point_count - (int)breadcrumbs->max_points);
    }
    return true;
}

static double _planet_breadcrumbs_distance(DcAppPlanetHandle planet, DcAppPlanetCrs crs, DcAppVec3d a, DcAppVec3d b) {
    plVec3d pa = {a.x, a.y, a.z};
    plVec3d pb = {b.x, b.y, b.z};

    if (crs == DC_APP_PLANET_CRS_GEODETIC && planet) {
        plVec3d ca = {0};
        plVec3d cb = {0};
        dc_geo_geodetic_to_cartesian_d(&planet->geodetic_crs, &planet->cartesian_crs, &pa, &ca, 1);
        dc_geo_geodetic_to_cartesian_d(&planet->geodetic_crs, &planet->cartesian_crs, &pb, &cb, 1);
        pa = ca;
        pb = cb;
    }

    double dx = pb.x - pa.x;
    double dy = pb.y - pa.y;
    double dz = pb.z - pa.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

static void _planet_ensure_initialized(DcAppPlanetContext *planet_ctx) {
    if (!planet_ctx || planet_ctx->extension_initialized) return;

    plPlanetExtInit init = {0};
    init.ptDevice = _ext_starter->get_device();
    _ext_planet->initialize(init);
    planet_ctx->extension_initialized = true;
}

static bool _planet_file_path_to_vfs(DcAppPlanetContext *planet_ctx, const char *path, char *out, size_t out_size) {
    if (!planet_ctx || !path || path[0] == '\0' || !out || out_size == 0) return false;

    char cleaned[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    strncpy(cleaned, path, sizeof(cleaned) - 1);
    dc_utils_trim_whitespace_inplace(cleaned);
    if (cleaned[0] == '\0') return false;

    if (cleaned[0] == '/' && _ext_vfs->does_file_exist(cleaned)) {
        strncpy(out, cleaned, out_size - 1);
        out[out_size - 1] = '\0';
        return true;
    }

    char abs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (dc_utils_is_relative_path(cleaned)) {
        const char *base_dir = planet_ctx->asset_root;
        if (!base_dir || base_dir[0] == '\0') return false;
        char joined[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
        if (dc_utils_join_paths(base_dir, cleaned, joined, sizeof(joined)) != 0) return false;
        if (dc_utils_canonicalize_path(joined, abs_path, sizeof(abs_path)) != 0) return false;
    } else if (dc_utils_canonicalize_path(cleaned, abs_path, sizeof(abs_path)) != 0) {
        return false;
    }

    char dir[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    dc_utils_get_directory(abs_path, dir, sizeof(dir));

    // Mount the containing directory under a stable hash for VFS-based planet loading.
    char hash[32] = {0};
    dc_utils_string_to_hash(dir, hash, sizeof(hash));

    char vfs_mount[33] = {0};
    snprintf(vfs_mount, sizeof(vfs_mount), "/%s", hash);
    _ext_vfs->mount_directory(vfs_mount, dir, PL_VFS_MOUNT_FLAGS_NONE);

    const char *fslash = strrchr(abs_path, '/');
    const char *bslash = strrchr(abs_path, '\\');
    const char *separator = fslash;
    if (!separator || (bslash && bslash > separator)) separator = bslash;
    const char *filename = separator ? separator + 1 : abs_path;
    snprintf(out, out_size, "%s/%s", vfs_mount, filename);
    return _ext_vfs->does_file_exist(out);
}

static bool _planet_file_path_to_absolute(DcAppPlanetContext *planet_ctx, const char *path, char *out, size_t out_size) {
    if (!planet_ctx || !path || path[0] == '\0' || !out || out_size == 0) return false;

    char cleaned[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    strncpy(cleaned, path, sizeof(cleaned) - 1);
    dc_utils_trim_whitespace_inplace(cleaned);
    if (cleaned[0] == '\0') return false;

    char joined[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (dc_utils_is_relative_path(cleaned)) {
        const char *base_dir = planet_ctx->asset_root;
        if (!base_dir || base_dir[0] == '\0') return false;
        if (dc_utils_join_paths(base_dir, cleaned, joined, sizeof(joined)) != 0) return false;
    } else {
        strncpy(joined, cleaned, sizeof(joined) - 1);
    }

    return dc_utils_canonicalize_path(joined, out, out_size) == 0;
}

static DcAppPlanetViewHandle _planet_create_view(DcAppPlanetContext *planet_ctx, DcAppPlanetHandle planet, DcAppPlanetCrs crs, uint32_t width, uint32_t height) {
    if (!planet_ctx || !planet || !planet->planet) return NULL;

    _planet_ensure_initialized(planet_ctx);
    if (sbcount(planet_ctx->sb_views) > UINT8_MAX) {
        DC_LOG_ERROR("PlanetView", "Too many planet views; dcapp supports at most %u planet view handles", UINT8_MAX);
        return NULL;
    }

    plPlanetViewInit view_init = {0};
    view_init.uOutputWidth = width > 0 ? width : 1024;
    view_init.uOutputHeight = height > 0 ? height : 1024;

    plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
    plPlanetView *view = _ext_planet->create_view(planet->planet, cmd_buf, view_init);
    _ext_starter->submit_temporary_command_buffer(cmd_buf);
    if (!view) return NULL;

    DcAppPlanetViewHandle handle = (DcAppPlanetViewHandle)PL_ALLOC(sizeof(*handle));
    memset(handle, 0, sizeof(*handle));
    handle->owner = planet_ctx;
    handle->planet = planet;
    handle->view = view;
    handle->crs = crs;
    handle->width = view_init.uOutputWidth;
    handle->height = view_init.uOutputHeight;
    sbpush(planet_ctx->sb_views, view);
    handle->index = (uint8_t)(sbcount(planet_ctx->sb_views) - 1);
    sbpush(planet_ctx->sb_view_handles, handle);
    return handle;
}

static bool _planet_load_process_info(const char *json_path, double *out_radius, plPlanetProcessInfo *out_info, bool *out_legacy_projected_origin) {
    char *json_str = dc_utils_load_text_file(json_path);
    if (!json_str) {
        DC_LOG_ERROR("Planet", "Failed to load planet data: %s", json_path);
        return false;
    }

    plJsonObject *root = NULL;
    if (!pl_load_json(json_str, &root)) {
        DC_LOG_ERROR("Planet", "Failed to parse planet data: %s", json_path);
        free(json_str);
        return false;
    }

    double radius           = pl_json_double_member(root, "radius", 0.0);
    float  meters_per_pixel = pl_json_float_member(root, "meters_per_pixel", 0.0f);
    int    tile_size        = pl_json_int_member(root, "tile_size", 0);
    int    cols             = pl_json_int_member(root, "cols", 0);
    int    rows             = pl_json_int_member(root, "rows", 0);
    float  min_height       = pl_json_float_member(root, "min_height", 0.0f);
    float  max_height       = pl_json_float_member(root, "max_height", 0.0f);
    int    tree_depth       = pl_json_int_member(root, "tree_depth", 0);
    float  max_base_error   = pl_json_float_member(root, "max_base_error", 0.0f);
    uint32_t tile_count     = 0;
    plJsonObject *tile_array = pl_json_array_member(root, "tiles", &tile_count);

    if (radius <= 0.0 || meters_per_pixel <= 0.0f || tile_size <= 0 || cols <= 0 || rows <= 0 || !tile_array || tile_count == 0) {
        DC_LOG_ERROR("Planet", "Invalid planet data: %s", json_path);
        pl_unload_json(&root);
        free(json_str);
        return false;
    }
    uint64_t expected_tile_count = (uint64_t)cols * (uint64_t)rows;
    if (expected_tile_count > UINT32_MAX || tile_count != (uint32_t)expected_tile_count) {
        DC_LOG_ERROR("Planet", "Planet tile count mismatch in %s: cols=%d rows=%d tiles=%u", json_path, cols, rows, tile_count);
        pl_unload_json(&root);
        free(json_str);
        return false;
    }

    memset(out_info, 0, sizeof(*out_info));
    out_info->tProjection.tType = PL_PROJECTION_POLAR_STEREOGRAPHIC;
    out_info->tProjection.tPolarStereo.dLatitudeOfOrigin = -90.0;
    out_info->tProjection.tPolarStereo.dLongitudeOfOrigin = 0.0;
    out_info->tProjection.tPolarStereo.dScaleFactor = 1.0;
    out_info->tProjection.tPolarStereo.dFalseEasting = 0.0;
    out_info->tProjection.tPolarStereo.dFalseNorthing = 0.0;
    plJsonObject *projection_obj = pl_json_member(root, "projection");
    // Pre-projection .planet.json files omitted this block and stored tile centers
    // as legacy lat/lon values instead of explicit projected CRS meters.
    bool legacy_projected_origin = projection_obj == NULL;
    if (out_legacy_projected_origin)
        *out_legacy_projected_origin = legacy_projected_origin;
    if (projection_obj) {
        char projection_type[64] = {0};
        pl_json_string_member(projection_obj, "type", projection_type, sizeof(projection_type));
        if (projection_type[0] && strcmp(projection_type, "polar_stereographic") != 0) {
            DC_LOG_ERROR("Planet", "Unsupported planet projection '%s' in %s", projection_type, json_path);
            pl_unload_json(&root);
            free(json_str);
            return false;
        }
        out_info->tProjection.tPolarStereo.dLatitudeOfOrigin =
            pl_json_double_member(projection_obj, "latitude_of_origin", out_info->tProjection.tPolarStereo.dLatitudeOfOrigin);
        out_info->tProjection.tPolarStereo.dLongitudeOfOrigin =
            pl_json_double_member(projection_obj, "longitude_of_origin", out_info->tProjection.tPolarStereo.dLongitudeOfOrigin);
        out_info->tProjection.tPolarStereo.dScaleFactor =
            pl_json_double_member(projection_obj, "scale_factor", out_info->tProjection.tPolarStereo.dScaleFactor);
        out_info->tProjection.tPolarStereo.dFalseEasting =
            pl_json_double_member(projection_obj, "false_easting", out_info->tProjection.tPolarStereo.dFalseEasting);
        out_info->tProjection.tPolarStereo.dFalseNorthing =
            pl_json_double_member(projection_obj, "false_northing", out_info->tProjection.tPolarStereo.dFalseNorthing);
    }
    if (fabs(out_info->tProjection.tPolarStereo.dLatitudeOfOrigin) < 45.0 ||
        out_info->tProjection.tPolarStereo.dScaleFactor <= 0.0) {
        DC_LOG_ERROR("Planet", "Invalid polar stereographic projection in %s", json_path);
        pl_unload_json(&root);
        free(json_str);
        return false;
    }
    out_info->tGeodeticModel.tDatum = PL_DATUM_SPHERE;
    out_info->tGeodeticModel.sphere.dRadius = radius;
    if (pl_json_bool_member(root, "double_precision", false))
        out_info->tFlags |= PL_PLANET_PROCESSING_FLAGS_DOUBLE_PRECISION;
    out_info->dMetersPerPixel = meters_per_pixel;
    out_info->uSize = (uint32_t)tile_size;
    out_info->uTileCount = tile_count;
    out_info->uHorizontalTiles = (uint32_t)cols;
    out_info->uVerticalTiles = (uint32_t)rows;

    // uses the same processed planet json format as xml planets.
    DcGeoCrsGeodetic geodetic_crs = dc_geo_create_crs_geodetic(radius);
    DcGeoCrsPolarStereo polar_crs = dc_geo_create_crs_polar_stereographic(
        radius,
        out_info->tProjection.tPolarStereo.dLatitudeOfOrigin,
        out_info->tProjection.tPolarStereo.dLongitudeOfOrigin);
    polar_crs.scale_factor = out_info->tProjection.tPolarStereo.dScaleFactor;
    polar_crs.false_easting = out_info->tProjection.tPolarStereo.dFalseEasting;
    polar_crs.false_northing = out_info->tProjection.tPolarStereo.dFalseNorthing;
    if ((size_t)tile_count > ((size_t)-1) / sizeof(plPlanetProcessTileInfo)) {
        DC_LOG_ERROR("Planet", "Planet data has too many tiles: %s", json_path);
        pl_unload_json(&root);
        free(json_str);
        return false;
    }

    out_info->atTiles = (plPlanetProcessTileInfo *)PL_ALLOC(tile_count * sizeof(plPlanetProcessTileInfo));
    if (!out_info->atTiles) {
        DC_LOG_ERROR("Planet", "Failed to allocate planet tile metadata: %s", json_path);
        pl_unload_json(&root);
        free(json_str);
        return false;
    }

    char json_dir[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_directory(json_path, json_dir, sizeof(json_dir));

    for (uint32_t t = 0; t < tile_count; t++) {
        plJsonObject *tile_obj = pl_json_member_by_index(tile_array, t);
        plPlanetProcessTileInfo *tile = &out_info->atTiles[t];
        memset(tile, 0, sizeof(*tile));

        if (pl_json_member_exist(tile_obj, "originX") && pl_json_member_exist(tile_obj, "originY")) {
            tile->dOriginX = pl_json_double_member(tile_obj, "originX", 0.0);
            tile->dOriginY = pl_json_double_member(tile_obj, "originY", 0.0);
        } else if (pl_json_member_exist(tile_obj, "lat") && pl_json_member_exist(tile_obj, "lon")) {
            plVec3d geodetic_in = {
                pl_json_double_member(tile_obj, "lat", 0.0),
                pl_json_double_member(tile_obj, "lon", 0.0),
                0.0
            };
            plVec2d polar_out;
            if (legacy_projected_origin) {
                // Compatibility path for old lat/lon tile metadata. New metadata
                // should provide originX/originY directly in projected CRS meters.
                dc_geo_user_geodetic_to_polar_stereo_d(&geodetic_crs, &polar_crs, &geodetic_in, &polar_out, 1);
                polar_out.y = -polar_out.y;
            } else {
                dc_geo_geodetic_to_polar_stereo_d(&geodetic_crs, &polar_crs, &geodetic_in, &polar_out, 1);
            }
            tile->dOriginX = polar_out.x;
            tile->dOriginY = polar_out.y;
        }

        tile->dMaxBaseError = (double)max_base_error;
        tile->dMaxHeight = (double)max_height;
        tile->dMinHeight = (double)min_height;
        tile->iTreeDepth = tree_depth;

        char chunk_file[256] = {0};
        pl_json_string_member(tile_obj, "file", chunk_file, sizeof(chunk_file));
        char abs_chunk_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
        if (dc_utils_is_relative_path(chunk_file))
            dc_utils_join_paths(json_dir, chunk_file, abs_chunk_path, sizeof(abs_chunk_path));
        else
            strncpy(abs_chunk_path, chunk_file, sizeof(abs_chunk_path) - 1);
        strncpy(tile->acOutputFile, abs_chunk_path, sizeof(tile->acOutputFile) - 1);
    }

    *out_radius = radius;
    pl_unload_json(&root);
    free(json_str);
    return true;
}

static void _planet_free_process_info(plPlanetProcessInfo *info) {
    if (!info) return;
    if (info->atTiles) PL_FREE(info->atTiles);
    memset(info, 0, sizeof(*info));
}
