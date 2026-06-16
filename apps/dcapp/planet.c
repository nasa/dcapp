#include "planet.h"

#include "geo.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/string.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static void _planet_ensure_initialized(_AppData *app_data);
static bool _planet_load_process_info(const char *json_path, double *out_radius, plPlanetProcessInfo *out_info, bool *out_legacy_projected_origin);
static void _planet_free_process_info(plPlanetProcessInfo *info);
static bool _planet_file_path_to_vfs(_AppData *app_data, const char *path, char *out, size_t out_size);
static DcAppPlanetViewHandle _planet_create_view(_AppData *app_data, DcAppPlanetHandle planet, DcAppPlanetCrs crs, uint32_t width, uint32_t height);
static void _planet_update_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3 position);
static float _planet_breadcrumbs_distance(DcAppPlanetHandle planet, DcAppPlanetCrs crs, DcAppVec3 a, DcAppVec3 b);

#define DC_APP_PLANET_MIN_MESH_CACHE_SIZE (1024u * 1024u)

// connects the public planet api table to the shared planet subsystem.
static const DcAppPlanetApi dc_app_planet_interface = {
    .get_planet_by_id       = dc_app_planet_get_planet_by_id,
    .create_planet          = dc_app_planet_create_planet,
    .create_planet_with_id  = dc_app_planet_create_planet_with_id,
    .set_texture_geodetic   = dc_app_planet_set_texture_geodetic,
    .set_texture_cartesian  = dc_app_planet_set_texture_cartesian,
    .create_geodetic_view   = dc_app_planet_create_geodetic_view,
    .create_cartesian_view  = dc_app_planet_create_cartesian_view,
    .set_view_shaders       = dc_app_planet_set_view_shaders,
    .create_breadcrumbs     = dc_app_planet_create_breadcrumbs,
    .update_breadcrumbs_geodetic  = dc_app_planet_update_breadcrumbs_geodetic,
    .update_breadcrumbs_cartesian = dc_app_planet_update_breadcrumbs_cartesian,
    .clear_breadcrumbs      = dc_app_planet_clear_breadcrumbs,
    .get_breadcrumbs_points = dc_app_planet_get_breadcrumbs_points,
};

const DcAppPlanetApi *dc_app_planet_api(void) {
    return &dc_app_planet_interface;
}

DcAppPlanetHandle dc_app_planet_get_planet_by_id(_AppData *app_data, const char *id) {
    if (!app_data || !id || id[0] == '\0') return NULL;

    for (int i = 0; i < sbcount(app_data->sb_planet_handles); i++) {
        DcAppPlanetHandle planet = app_data->sb_planet_handles[i];
        if (planet && planet->id && strcmp(planet->id, id) == 0) return planet;
    }

    return NULL;
}

DcAppPlanetHandle dc_app_planet_create_planet(_AppData *app_data, DcAppPlanetCreateInfo info) {
    if (!app_data || !info.data_path || info.data_path[0] == '\0') return NULL;

    // initializes the planet extension on first use.
    _planet_ensure_initialized(app_data);
    if (sbcount(app_data->sb_planets) > UINT8_MAX) {
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

    uint32_t mesh_cache_size = info.mesh_cache_size;
    if (mesh_cache_size > 0 && mesh_cache_size < DC_APP_PLANET_MIN_MESH_CACHE_SIZE) {
        DC_LOG_WARN("Planet", "mesh_cache_size is %u bytes; using renderer default instead", mesh_cache_size);
        mesh_cache_size = 0;
    }
    if (mesh_cache_size > 0) {
        planet_init.uVertexBufferSize = mesh_cache_size / 2;
        planet_init.uIndexBufferSize  = mesh_cache_size / 2;
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
    handle->app_data = app_data;
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
    sbpush(app_data->sb_planets, planet);
    handle->index = (uint8_t)(sbcount(app_data->sb_planets) - 1);
    sbpush(app_data->sb_planet_handles, handle);
    return handle;
}

DcAppPlanetHandle dc_app_planet_create_planet_with_id(_AppData *app_data, const char *id, DcAppPlanetCreateInfo info) {
    if (!app_data || !id || id[0] == '\0') return NULL;
    if (dc_app_planet_get_planet_by_id(app_data, id)) {
        DC_LOG_ERROR("Planet", "Planet id already exists: %s", id);
        return NULL;
    }

    DcAppPlanetHandle planet = dc_app_planet_create_planet(app_data, info);
    if (planet) {
        size_t len = strlen(id) + 1;
        planet->id = (char *)PL_ALLOC(len);
        memcpy(planet->id, id, len);
    }
    return planet;
}

bool dc_app_planet_set_texture_geodetic(_AppData *app_data, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel) {
    if (!app_data || !planet || !planet->planet || meters_per_pixel <= 0.0f) return false;

    char vfs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (!_planet_file_path_to_vfs(app_data, path, vfs_path, sizeof(vfs_path))) return false;

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
    _ext_planet->set_texture(planet->planet, &texture, 0);
    return true;
}

bool dc_app_planet_set_texture_cartesian(_AppData *app_data, DcAppPlanetHandle planet, const char *path, DcAppVec3 position, float meters_per_pixel) {
    if (!app_data || !planet || !planet->planet || meters_per_pixel <= 0.0f) return false;

    char vfs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    if (!_planet_file_path_to_vfs(app_data, path, vfs_path, sizeof(vfs_path))) return false;

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
    _ext_planet->set_texture(planet->planet, &texture, 0);
    return true;
}

DcAppPlanetViewHandle dc_app_planet_create_geodetic_view(_AppData *app_data, DcAppPlanetHandle planet, uint32_t width, uint32_t height) {
    return _planet_create_view(app_data, planet, DC_APP_PLANET_CRS_GEODETIC, width, height);
}

DcAppPlanetViewHandle dc_app_planet_create_cartesian_view(_AppData *app_data, DcAppPlanetHandle planet, uint32_t width, uint32_t height) {
    return _planet_create_view(app_data, planet, DC_APP_PLANET_CRS_CARTESIAN, width, height);
}

bool dc_app_planet_set_view_shaders(DcAppPlanetViewHandle view, const char *vertex_shader, const char *fragment_shader) {
    if (!view || !view->view) return false;

    char vertex_vfs[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    char fragment_vfs[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    const char *vertex_path = NULL;
    const char *fragment_path = NULL;

    if (vertex_shader && vertex_shader[0] != '\0') {
        if (!_planet_file_path_to_vfs(view->app_data, vertex_shader, vertex_vfs, sizeof(vertex_vfs))) return false;
        vertex_path = vertex_vfs;
    }
    if (fragment_shader && fragment_shader[0] != '\0') {
        if (!_planet_file_path_to_vfs(view->app_data, fragment_shader, fragment_vfs, sizeof(fragment_vfs))) return false;
        fragment_path = fragment_vfs;
    }

    _ext_planet->set_shaders(view->view, vertex_path, fragment_path);
    return true;
}

DcAppPlanetBreadcrumbsHandle dc_app_planet_create_breadcrumbs(_AppData *app_data, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing) {
    if (!app_data) return NULL;
    if (crs != DC_APP_PLANET_CRS_GEODETIC && crs != DC_APP_PLANET_CRS_CARTESIAN) return NULL;
    if (max_points < 2) max_points = 2;
    if (point_spacing < 0.0f) point_spacing = 0.0f;

    DcAppPlanetBreadcrumbsHandle breadcrumbs = (DcAppPlanetBreadcrumbsHandle)PL_ALLOC(sizeof(*breadcrumbs));
    if (!breadcrumbs) return NULL;
    memset(breadcrumbs, 0, sizeof(*breadcrumbs));
    breadcrumbs->app_data = app_data;
    breadcrumbs->crs = crs;
    breadcrumbs->max_points = max_points;
    breadcrumbs->point_spacing = point_spacing;
    sbpush(app_data->sb_planet_breadcrumbs, breadcrumbs);
    return breadcrumbs;
}

void dc_app_planet_update_breadcrumbs_geodetic(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3 position) {
    if (!breadcrumbs || breadcrumbs->crs != DC_APP_PLANET_CRS_GEODETIC || !planet) return;
    _planet_update_breadcrumbs(breadcrumbs, planet, position);
}

void dc_app_planet_update_breadcrumbs_cartesian(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppVec3 position) {
    if (!breadcrumbs || breadcrumbs->crs != DC_APP_PLANET_CRS_CARTESIAN) return;
    _planet_update_breadcrumbs(breadcrumbs, NULL, position);
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

void dc_app_planet_free_wrappers(_AppData *app_data) {
    if (!app_data) return;

    for (int i = 0; i < sbcount(app_data->sb_planet_breadcrumbs); i++) {
        DcAppPlanetBreadcrumbsHandle breadcrumbs = app_data->sb_planet_breadcrumbs[i];
        if (!breadcrumbs) continue;
        sbfree(breadcrumbs->sb_points);
        PL_FREE(breadcrumbs);
    }
    sbfree(app_data->sb_planet_breadcrumbs);

    // cleans up every planet view created by the shared planet subsystem.
    for (int i = 0; i < sbcount(app_data->sb_planet_view_handles); i++) {
        DcAppPlanetViewHandle view = app_data->sb_planet_view_handles[i];
        if (!view) continue;
        if (view->view) _ext_planet->cleanup_view(view->view);
        PL_FREE(view);
    }
    sbfree(app_data->sb_planet_view_handles);

    // cleans up every planet created by the shared planet subsystem.
    for (int i = 0; i < sbcount(app_data->sb_planet_handles); i++) {
        DcAppPlanetHandle planet = app_data->sb_planet_handles[i];
        if (!planet) continue;
        if (planet->planet) _ext_planet->cleanup_planet(planet->planet);
        if (planet->id) PL_FREE(planet->id);
        PL_FREE(planet);
    }
    sbfree(app_data->sb_planet_handles);
    sbfree(app_data->sb_planet_views);
    sbfree(app_data->sb_planets);
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

static void _planet_update_breadcrumbs(DcAppPlanetBreadcrumbsHandle breadcrumbs, DcAppPlanetHandle planet, DcAppVec3 position) {
    if (!isfinite(position.x) || !isfinite(position.y) || !isfinite(position.z)) return;

    int point_count = sbcount(breadcrumbs->sb_points);
    if (point_count == 0 ||
        _planet_breadcrumbs_distance(planet, breadcrumbs->crs, breadcrumbs->sb_points[point_count - 1], position) >= breadcrumbs->point_spacing) {
        sbpush(breadcrumbs->sb_points, position);
    }

    point_count = sbcount(breadcrumbs->sb_points);
    if (point_count > (int)breadcrumbs->max_points) {
        sbshiftn(breadcrumbs->sb_points, point_count - (int)breadcrumbs->max_points);
    }
}

static float _planet_breadcrumbs_distance(DcAppPlanetHandle planet, DcAppPlanetCrs crs, DcAppVec3 a, DcAppVec3 b) {
    plVec3 pa = {a.x, a.y, a.z};
    plVec3 pb = {b.x, b.y, b.z};

    if (crs == DC_APP_PLANET_CRS_GEODETIC && planet) {
        plVec3 ca = {0};
        plVec3 cb = {0};
        dc_geo_geodetic_to_cartesian(&planet->geodetic_crs, &planet->cartesian_crs, &pa, &ca, 1);
        dc_geo_geodetic_to_cartesian(&planet->geodetic_crs, &planet->cartesian_crs, &pb, &cb, 1);
        pa = ca;
        pb = cb;
    }

    float dx = pb.x - pa.x;
    float dy = pb.y - pa.y;
    float dz = pb.z - pa.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static void _planet_ensure_initialized(_AppData *app_data) {
    if (!app_data || app_data->planet_ext_initialized) return;

    plPlanetExtInit init = {0};
    init.ptDevice = _ext_starter->get_device();
    _ext_planet->initialize(init);
    app_data->planet_ext_initialized = true;

    // keeps xml lookup arrays in their initialized one-based shape.
    sbpush(app_data->sb_planets, NULL);
    sbpush(app_data->sb_planet_views, NULL);
}

static bool _planet_file_path_to_vfs(_AppData *app_data, const char *path, char *out, size_t out_size) {
    if (!app_data || !path || path[0] == '\0' || !out || out_size == 0) return false;

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
        const char *base_dir = app_data->config ? app_data->config->config_dir_path : NULL;
        if (!base_dir || base_dir[0] == '\0') return false;
        char joined[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
        if (dc_utils_join_paths(base_dir, cleaned, joined, sizeof(joined)) != 0) return false;
        if (dc_utils_canonicalize_path(joined, abs_path, sizeof(abs_path)) != 0) return false;
    } else if (dc_utils_canonicalize_path(cleaned, abs_path, sizeof(abs_path)) != 0) {
        return false;
    }

    char dir[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    dc_utils_get_directory(abs_path, dir, sizeof(dir));

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

static DcAppPlanetViewHandle _planet_create_view(_AppData *app_data, DcAppPlanetHandle planet, DcAppPlanetCrs crs, uint32_t width, uint32_t height) {
    if (!app_data || !planet || !planet->planet) return NULL;

    _planet_ensure_initialized(app_data);
    if (sbcount(app_data->sb_planet_views) > UINT8_MAX) {
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
    handle->app_data = app_data;
    handle->planet = planet;
    handle->view = view;
    handle->crs = crs;
    handle->width = view_init.uOutputWidth;
    handle->height = view_init.uOutputHeight;
    sbpush(app_data->sb_planet_views, view);
    handle->index = (uint8_t)(sbcount(app_data->sb_planet_views) - 1);
    sbpush(app_data->sb_planet_view_handles, handle);
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

    char json_dir[DC_VALUE_STRING_BUFFER_SIZE];
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
        char abs_chunk_path[DC_VALUE_STRING_BUFFER_SIZE] = {0};
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
