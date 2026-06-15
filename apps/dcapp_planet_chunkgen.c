/*
   dcapp_planet_chunkgen.c

   Pilotlight app that converts a GeoTIFF DEM into .chu chunk files
   for the planet rendering pipeline.

   Usage:
     pilot_light -a dcapp-planet-chunkgen <input_dem> <output_dir> [options]
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] structs
// [SECTION] forward declarations
// [SECTION] extension globals
// [SECTION] pl_app_load
// [SECTION] pl_app_update
// [SECTION] pl_app_shutdown
// [SECTION] helpers
// [SECTION] GDAL helpers
// [SECTION] stereographic projection
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <gdal.h>
#include <gdal_utils.h>
#include <ogr_srs_api.h>
#include <cpl_conv.h>
#include "pl.h"
#include "../src/utils/file.h"
#include "pl_planet_processor_ext.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _DcPlanetDemInfo {
    uint32_t width;
    uint32_t height;
    double gt_m[6];
    double pixel_scale;
    double radius;
    double band_scale;
    plProjectionParams projection;
} DcPlanetDemInfo;

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

static bool  _parse_gdalinfo(const char *dem_path, DcPlanetDemInfo *info);
static bool  _parse_gdalinfo_mm(const char *dem_path, double *min_val, double *max_val);
static bool  _write_height_tile(const char *input, const char *output, uint32_t src_x, uint32_t src_y, uint32_t w, uint32_t h, uint32_t tile_size, double min_h, double max_h);
static void  _show_help(void);
static char *_get_stem(const char *path, char *buf, size_t buf_size);
static void  _pixel_to_projected_meters(const DcPlanetDemInfo *info, double pixel_x, double pixel_y, double *out_x, double *out_y);

//-----------------------------------------------------------------------------
// [SECTION] extension globals
//-----------------------------------------------------------------------------

static const plIOI              *_ext_ioi              = NULL;
static const plPlanetProcessorI *_ext_planet_processor = NULL;

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, void *app_data) {

    if (app_data)
        return app_data;

    // load extensions
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", "pl_load_platform_ext", "pl_unload_platform_ext", false);
    extension_registry->load("pl_planet_processor_ext", NULL, NULL, true);

    // fetch APIs
    _ext_ioi              = pl_get_api_latest(api_registry, plIOI);
    _ext_planet_processor = pl_get_api_latest(api_registry, plPlanetProcessorI);
    if (!_ext_planet_processor) {
        fprintf(stderr, "Error: failed to get plPlanetProcessorI from registry\n");
        plIO *io     = _ext_ioi->get_io();
        io->bRunning = false;
        return NULL;
    }

    // get args (argv[0]=pilot_light, argv[1]=-a, argv[2]=dcapp-planet-chunkgen, argv[3]+=app args)
    plIO  *io   = _ext_ioi->get_io();
    int    argc = io->iArgc - 3;
    char **argv = io->apArgv + 3;

    //---- parse arguments ----

    const char *input_dem        = NULL;
    const char *output_dir       = NULL;
    double      radius           = 0.0;
    uint32_t    tile_size        = 4096;
    double      min_height       = NAN;
    double      max_height       = NAN;
    double      meters_per_pixel = 0.0;
    int         tree_depth       = 6;
    float       max_base_error   = 0.0f;
    const char *prefix           = NULL;
    bool        keep_tiles       = false;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            _show_help();
            io->bRunning = false;
            return NULL;
        } else if (strcmp(argv[i], "--radius") == 0 && i + 1 < argc)
            radius = atof(argv[++i]);
        else if (strcmp(argv[i], "--tile-size") == 0 && i + 1 < argc)
            tile_size = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--min-height") == 0 && i + 1 < argc)
            min_height = atof(argv[++i]);
        else if (strcmp(argv[i], "--max-height") == 0 && i + 1 < argc)
            max_height = atof(argv[++i]);
        else if (strcmp(argv[i], "--meters-per-pixel") == 0 && i + 1 < argc)
            meters_per_pixel = atof(argv[++i]);
        else if (strcmp(argv[i], "--tree-depth") == 0 && i + 1 < argc)
            tree_depth = atoi(argv[++i]);
        else if (strcmp(argv[i], "--max-base-error") == 0 && i + 1 < argc)
            max_base_error = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc)
            prefix = argv[++i];
        else if (strcmp(argv[i], "--keep-tiles") == 0)
            keep_tiles = true;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: unknown option: %s\n", argv[i]);
            io->bRunning = false;
            return NULL;
        } else if (!input_dem)
            input_dem = argv[i];
        else if (!output_dir)
            output_dir = argv[i];
        else {
            fprintf(stderr, "Error: unexpected argument: %s\n", argv[i]);
            io->bRunning = false;
            return NULL;
        }
    }

    //---- validate ----

    if (!input_dem || !output_dir) {
        fprintf(stderr, "Error: input_dem and output_dir are required\n");
        _show_help();
        io->bRunning = false;
        return NULL;
    }

    if (!dc_utils_file_exists(input_dem)) {
        fprintf(stderr, "Error: input file not found: %s\n", input_dem);
        io->bRunning = false;
        return NULL;
    }

    GDALAllRegister();
    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");

    // derive prefix
    char prefix_buf[256];
    if (!prefix) {
        _get_stem(input_dem, prefix_buf, sizeof(prefix_buf));
        prefix = prefix_buf;
    }

    //---- get DEM properties ----

    printf("========================================\n");
    printf("dcapp-planet-chunkgen\n");
    printf("========================================\n");
    printf("Input: %s\n", input_dem);

    DcPlanetDemInfo dem_info = {0};
    if (!_parse_gdalinfo(input_dem, &dem_info)) {
        io->bRunning = false;
        return NULL;
    }

    // use DEM radius if not overridden
    if (radius <= 0.0) {
        if (dem_info.radius > 0.0) {
            radius = dem_info.radius;
            printf("Auto-detected radius: %.1f m\n", radius);
        } else {
            fprintf(stderr, "Error: could not detect radius from DEM; use --radius\n");
            io->bRunning = false;
            return NULL;
        }
    }

    printf("Raster size: %u x %u\n", dem_info.width, dem_info.height);
    printf("Radius: %.1f m\n", radius);
    printf("Pixel scale: %.6f m\n", dem_info.pixel_scale);
    printf("Projection: %s lat0=%.3f lon0=%.3f k0=%.8f false=(%.3f, %.3f)\n",
           "polar_stereographic",
           dem_info.projection.tPolarStereo.dLatitudeOfOrigin,
           dem_info.projection.tPolarStereo.dLongitudeOfOrigin,
           dem_info.projection.tPolarStereo.dScaleFactor,
           dem_info.projection.tPolarStereo.dFalseEasting,
           dem_info.projection.tPolarStereo.dFalseNorthing);
    if (dem_info.band_scale != 1.0)
        printf("DEM band scale: %.6f\n", dem_info.band_scale);

    // auto-detect meters per pixel from DEM pixel scale
    if (meters_per_pixel <= 0.0) {
        if (dem_info.pixel_scale > 0.0) {
            meters_per_pixel = dem_info.pixel_scale;
            printf("Auto-detected meters/pixel: %.6f\n", meters_per_pixel);
        } else {
            fprintf(stderr, "Error: could not detect pixel scale; use --meters-per-pixel\n");
            io->bRunning = false;
            return NULL;
        }
    }

    // auto-detect elevation range
    if (isnan(min_height) || isnan(max_height)) {
        printf("Computing min/max elevation...\n");
        double detected_min = 0.0, detected_max = 0.0;
        if (!_parse_gdalinfo_mm(input_dem, &detected_min, &detected_max)) {
            io->bRunning = false;
            return NULL;
        }
        // GDALComputeRasterStatistics returns raw pixel values;
        // apply GDAL's band scale to convert to terrain meters.
        if (isnan(min_height))
            min_height = detected_min * dem_info.band_scale;
        if (isnan(max_height))
            max_height = detected_max * dem_info.band_scale;
    }

    printf("Elevation range: %.2f to %.2f m\n", min_height, max_height);

    // default max_base_error from meters per pixel if not specified
    if (max_base_error <= 0.0f)
        max_base_error = 0.15f * (float)meters_per_pixel;
    printf("Max base error: %.2f\n", max_base_error);

    //---- compute tile grid ----

    uint32_t cols = (dem_info.width + tile_size - 1) / tile_size;
    uint32_t rows = (dem_info.height + tile_size - 1) / tile_size;
    uint64_t tile_count64 = (uint64_t)cols * (uint64_t)rows;
    if (tile_count64 == 0 || tile_count64 > UINT32_MAX) {
        fprintf(stderr, "Error: tile grid is too large: %u x %u\n", cols, rows);
        io->bRunning = false;
        return NULL;
    }
    uint32_t tile_count = (uint32_t)tile_count64;

    printf("Tile size: %u px\n", tile_size);
    printf("Grid: %u x %u (%u tiles)\n", cols, rows, tile_count);
    printf("Tree depth: %d\n", tree_depth);
    printf("Prefix: %s\n", prefix);
    printf("========================================\n\n");

    //---- create output directory ----

    dc_utils_create_directory(output_dir);

    //---- setup tile info ----

    plPlanetProcessTileInfo *tiles = calloc(tile_count, sizeof(plPlanetProcessTileInfo));
    if (!tiles) {
        fprintf(stderr, "Error: failed to allocate tile info\n");
        io->bRunning = false;
        return NULL;
    }

    plPlanetProcessInfo planet_info = {
        .tProjection = dem_info.projection,
        .tGeodeticModel = {
            .tDatum = PL_DATUM_SPHERE,
            .sphere = {
                .dRadius = radius
            }
        },
        .dMetersPerPixel  = meters_per_pixel,
        .uSize            = tile_size,
        .uTileCount       = tile_count,
        .atTiles          = tiles,
        .uHorizontalTiles = cols,
        .uVerticalTiles   = rows,
    };

    for (uint32_t row = 0; row < rows; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            uint32_t idx = col + row * cols;

            tiles[idx].iTreeDepth    = tree_depth;
            tiles[idx].dMaxHeight    = max_height;
            tiles[idx].dMinHeight    = min_height;
            tiles[idx].dMaxBaseError = (double)max_base_error;

            double tile_center_x = 0.0;
            double tile_center_y = 0.0;
            _pixel_to_projected_meters(&dem_info,
                                       (double)col * tile_size + (double)tile_size * 0.5,
                                       (double)row * tile_size + (double)tile_size * 0.5,
                                       &tile_center_x, &tile_center_y);

            tiles[idx].dOriginX = tile_center_x;
            tiles[idx].dOriginY = tile_center_y;

            // file paths (real filesystem paths)
            snprintf(tiles[idx].acHeightMapFile, 256, "%s/%s_%u_%u.png", output_dir, prefix, col, row);
            snprintf(tiles[idx].acOutputFile, 256, "%s/%s_%u_%u.chu", output_dir, prefix, col, row);
        }
    }

    //---- write metadata ----

    printf("[Step 1/%d] Writing metadata...\n", 3);

    char meta_path[2048];
    snprintf(meta_path, sizeof(meta_path), "%s/%s.planet.json", output_dir, prefix);

    plJsonObject *root = pl_json_new_root_object("root");

    pl_json_add_double_member(root, "radius", radius);
    pl_json_add_double_member(root, "meters_per_pixel", meters_per_pixel);
    pl_json_add_int_member(root, "tile_size", (int)tile_size);
    pl_json_add_int_member(root, "cols", (int)cols);
    pl_json_add_int_member(root, "rows", (int)rows);
    pl_json_add_double_member(root, "min_height", min_height);
    pl_json_add_double_member(root, "max_height", max_height);
    pl_json_add_int_member(root, "tree_depth", tree_depth);
    pl_json_add_float_member(root, "max_base_error", max_base_error);
    plJsonObject *projection_obj = pl_json_add_member(root, "projection");
    pl_json_add_string_member(projection_obj, "type", "polar_stereographic");
    pl_json_add_double_member(projection_obj, "latitude_of_origin", dem_info.projection.tPolarStereo.dLatitudeOfOrigin);
    pl_json_add_double_member(projection_obj, "longitude_of_origin", dem_info.projection.tPolarStereo.dLongitudeOfOrigin);
    pl_json_add_double_member(projection_obj, "scale_factor", dem_info.projection.tPolarStereo.dScaleFactor);
    pl_json_add_double_member(projection_obj, "false_easting", dem_info.projection.tPolarStereo.dFalseEasting);
    pl_json_add_double_member(projection_obj, "false_northing", dem_info.projection.tPolarStereo.dFalseNorthing);
    plJsonObject *tile_array = pl_json_add_member_array(root, "tiles", tile_count);
    for (uint32_t i = 0; i < tile_count; i++) {
        plJsonObject *tile_obj = pl_json_member_by_index(tile_array, i);

        const char *filename = strrchr(tiles[i].acOutputFile, '/');
        if (filename)
            filename++;
        else
            filename = tiles[i].acOutputFile;

        pl_json_add_double_member(tile_obj, "originX", tiles[i].dOriginX);
        pl_json_add_double_member(tile_obj, "originY", tiles[i].dOriginY);
        pl_json_add_string_member(tile_obj, "file", filename);
    }

    uint32_t buf_size = 0;
    pl_write_json(root, NULL, &buf_size);
    char *json_buf = malloc(buf_size + 1);
    pl_write_json(root, json_buf, &buf_size);
    json_buf[buf_size] = '\0';
    pl_unload_json(&root);

    FILE *meta_file = fopen(meta_path, "w");
    if (meta_file) {
        fwrite(json_buf, 1, buf_size, meta_file);
        fclose(meta_file);
        printf("Metadata: %s\n", meta_path);
    } else {
        fprintf(stderr, "Error: could not write metadata: %s\n", meta_path);
        free(json_buf);
        free(tiles);
        io->bRunning = false;
        return NULL;
    }
    free(json_buf);

    //---- tile DEM with GDAL ----

    // convert meter heights back to raw DEM values for -scale
    double raw_min_h = min_height / dem_info.band_scale;
    double raw_max_h = max_height / dem_info.band_scale;

    printf("\n[Step 2/%d] Tiling DEM into %u PNGs...\n", 3, tile_count);

    for (uint32_t t = 0; t < tile_count; t++) {
        uint32_t col = t % cols;
        uint32_t row = t / cols;
        uint32_t sx  = col * tile_size;
        uint32_t sy  = row * tile_size;
        uint32_t w   = tile_size;
        uint32_t h   = tile_size;
        if (sx + w > dem_info.width)
            w = dem_info.width - sx;
        if (sy + h > dem_info.height)
            h = dem_info.height - sy;

        char tile_path[2048];
        snprintf(tile_path, sizeof(tile_path), "%s/%s_%u_%u.png", output_dir, prefix, col, row);

        printf("  [%u/%u] %s_%u_%u.png\n", t + 1, tile_count, prefix, col, row);

        if (!_write_height_tile(input_dem, tile_path, sx, sy, w, h, tile_size, raw_min_h, raw_max_h)) {
            io->bRunning = false;
            return NULL;
        }
    }

    //---- process chunks ----

    for (uint32_t i = 0; i < tile_count; i++)
        remove(tiles[i].acOutputFile);

    printf("\n[Step 3/%d] Processing chunks...\n", 3);

    _ext_planet_processor->process(&planet_info);

    //---- cleanup ----

    if (!keep_tiles) {
        printf("Cleaning up intermediate PNGs...\n");
        for (uint32_t i = 0; i < tile_count; i++) {
            remove(tiles[i].acHeightMapFile);
        }
    }

    free(tiles);

    printf("\n========================================\n");
    printf("Done. %u chunk files written to %s/\n", tile_count, output_dir);
    printf("========================================\n");

    io->bRunning = false;
    return NULL;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void pl_app_update(void *app_data) {
    plIO *io     = _ext_ioi->get_io();
    io->bRunning = false;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void pl_app_shutdown(void *app_data) {}

//-----------------------------------------------------------------------------
// [SECTION] helpers
//-----------------------------------------------------------------------------

static void _show_help(void) {
    printf("dcapp-planet-chunkgen - Convert GeoTIFF DEM to planet chunk files\n\n");
    printf("Usage:\n");
    printf("  dcapp-planet-chunkgen <input_dem> <output_dir> [options]\n\n");
    printf("Options:\n");
    printf("  --radius N             Planet radius in meters (default: auto-detect from DEM)\n");
    printf("  --tile-size N          Tile dimensions in pixels (default: 4096)\n");
    printf("  --min-height N         Min elevation in meters (default: auto-detect)\n");
    printf("  --max-height N         Max elevation in meters (default: auto-detect)\n");
    printf("  --meters-per-pixel N   Meters per pixel (default: auto-detect from DEM)\n");
    printf("  --tree-depth N         CDLOD quadtree depth (default: 6)\n");
    printf("  --max-base-error N     LOD error threshold (default: 0.15 * meters_per_pixel)\n");
    printf("  --prefix NAME          Output naming prefix (default: input filename stem)\n");
    printf("  --keep-tiles           Don't delete intermediate PNG tiles\n");
    printf("  -h, --help             Show this help\n");
}

static char *_get_stem(const char *path, char *buf, size_t buf_size) {
    const char *fslash = strrchr(path, '/');
    const char *bslash = strrchr(path, '\\');
    const char *slash  = (fslash > bslash) ? fslash : bslash;
    const char *name   = slash ? slash + 1 : path;
    strncpy(buf, name, buf_size - 1);
    buf[buf_size - 1] = '\0';
    char *dot         = strrchr(buf, '.');
    if (dot)
        *dot = '\0';
    return buf;
}

//-----------------------------------------------------------------------------
// [SECTION] GDAL helpers
//-----------------------------------------------------------------------------

static bool _almost_zero(double v) {
    return fabs(v) < 1.0e-12;
}

static void _pixel_to_projected_meters(const DcPlanetDemInfo *info, double pixel_x, double pixel_y, double *out_x, double *out_y) {
    *out_x = info->gt_m[0] + pixel_x * info->gt_m[1] + pixel_y * info->gt_m[2];
    *out_y = info->gt_m[3] + pixel_x * info->gt_m[4] + pixel_y * info->gt_m[5];
}

static bool _parse_gdalinfo(const char *dem_path, DcPlanetDemInfo *info) {
    memset(info, 0, sizeof(*info));
    info->band_scale = 1.0;
    info->projection.tType = PL_PROJECTION_POLAR_STEREOGRAPHIC;
    info->projection.tPolarStereo.dLatitudeOfOrigin = -90.0;
    info->projection.tPolarStereo.dLongitudeOfOrigin = 0.0;
    info->projection.tPolarStereo.dScaleFactor = 1.0;

    GDALDatasetH ds = GDALOpen(dem_path, GA_ReadOnly);
    if (!ds) {
        fprintf(stderr, "Error: GDALOpen failed for: %s\n", dem_path);
        return false;
    }

    info->width  = (uint32_t)GDALGetRasterXSize(ds);
    info->height = (uint32_t)GDALGetRasterYSize(ds);
    if (info->width == 0 || info->height == 0) {
        fprintf(stderr, "Error: DEM has invalid raster size: %u x %u\n", info->width, info->height);
        GDALClose(ds);
        return false;
    }

    // get linear unit conversion factor (to meters)
    double               to_meters = 1.0;
    OGRSpatialReferenceH srs       = GDALGetSpatialRef(ds);
    if (!srs) {
        fprintf(stderr, "Error: DEM is missing spatial reference metadata; use a projected polar stereographic DEM\n");
        GDALClose(ds);
        return false;
    }

    to_meters = OSRGetLinearUnits(srs, NULL);

    OGRErr err;
    double semi_major = OSRGetSemiMajor(srs, &err);
    if (err == OGRERR_NONE && semi_major > 0.0)
        info->radius = semi_major;

    const char *projection_name = OSRGetAttrValue(srs, "PROJECTION", 0);
    if (!projection_name) {
        fprintf(stderr, "Error: DEM spatial reference is not projected polar stereographic\n");
        GDALClose(ds);
        return false;
    }
    if (strcmp(projection_name, SRS_PT_POLAR_STEREOGRAPHIC) != 0 &&
        strcmp(projection_name, SRS_PT_STEREOGRAPHIC) != 0) {
        fprintf(stderr, "Error: unsupported DEM projection '%s' (expected polar stereographic/UPS)\n", projection_name);
        GDALClose(ds);
        return false;
    }

    OGRErr proj_err = OGRERR_NONE;
    double lat_origin = OSRGetProjParm(srs, SRS_PP_LATITUDE_OF_ORIGIN, -90.0, &proj_err);
    OGRErr std_parallel_err = OGRERR_NONE;
    double standard_parallel = OSRGetProjParm(srs, SRS_PP_STANDARD_PARALLEL_1, lat_origin, &std_parallel_err);
    if (proj_err != OGRERR_NONE && std_parallel_err == OGRERR_NONE)
        lat_origin = standard_parallel;
    if (fabs(lat_origin) < 45.0) {
        fprintf(stderr, "Error: polar stereographic DEM does not identify a polar latitude of origin\n");
        GDALClose(ds);
        return false;
    }
    info->projection.tPolarStereo.dLatitudeOfOrigin = lat_origin >= 0.0 ? 90.0 : -90.0;

    OGRErr lon_err = OGRERR_NONE;
    double lon_origin = OSRGetProjParm(srs, SRS_PP_CENTRAL_MERIDIAN, 0.0, &lon_err);
    if (lon_err != OGRERR_NONE)
        lon_origin = OSRGetProjParm(srs, SRS_PP_LONGITUDE_OF_ORIGIN, 0.0, NULL);
    info->projection.tPolarStereo.dLongitudeOfOrigin = lon_origin;

    OGRErr scale_err = OGRERR_NONE;
    double scale_factor = OSRGetProjParm(srs, SRS_PP_SCALE_FACTOR, 1.0, &scale_err);
    if (scale_err != OGRERR_NONE) {
        double lat_ts = (std_parallel_err == OGRERR_NONE) ? standard_parallel : lat_origin;
        if (fabs(lat_ts) < 45.0) {
            fprintf(stderr, "Error: polar stereographic DEM is missing scale factor and standard parallel\n");
            GDALClose(ds);
            return false;
        }
        scale_factor = (fabs(fabs(lat_ts) - 90.0) <= 1.0e-9)
            ? 1.0
            : 0.5 * (1.0 + sin(fabs(lat_ts) * M_PI / 180.0));
    }
    if (scale_factor <= 0.0)
        scale_factor = 1.0;
    info->projection.tPolarStereo.dScaleFactor = scale_factor;
    info->projection.tPolarStereo.dFalseEasting = OSRGetProjParm(srs, SRS_PP_FALSE_EASTING, 0.0, NULL) * to_meters;
    info->projection.tPolarStereo.dFalseNorthing = OSRGetProjParm(srs, SRS_PP_FALSE_NORTHING, 0.0, NULL) * to_meters;

    double gt[6] = {0};
    if (GDALGetGeoTransform(ds, gt) != CE_None) {
        fprintf(stderr, "Error: DEM is missing a geotransform\n");
        GDALClose(ds);
        return false;
    }
    if (!_almost_zero(gt[2]) || !_almost_zero(gt[4])) {
        fprintf(stderr, "Error: rotated/skewed DEM geotransforms are not supported yet\n");
        GDALClose(ds);
        return false;
    }

    for (int i = 0; i < 6; i++)
        info->gt_m[i] = gt[i] * to_meters;

    double pixel_scale_x = fabs(info->gt_m[1]);
    double pixel_scale_y = fabs(info->gt_m[5]);
    if (pixel_scale_x <= 0.0 || pixel_scale_y <= 0.0) {
        fprintf(stderr, "Error: invalid DEM pixel scale\n");
        GDALClose(ds);
        return false;
    }
    if (fabs(pixel_scale_x - pixel_scale_y) > pixel_scale_x * 1.0e-6) {
        fprintf(stderr, "Error: non-square DEM pixels are not supported (%.9f x %.9f m)\n", pixel_scale_x, pixel_scale_y);
        GDALClose(ds);
        return false;
    }
    info->pixel_scale = pixel_scale_x;

    // raster band scale/offset for elevation unit conversion
    // real_value = offset + raw_pixel * scale
    GDALRasterBandH band = GDALGetRasterBand(ds, 1);
    if (band) {
        int bHasScale = 0;
        double band_scale = GDALGetRasterScale(band, &bHasScale);
        if (!bHasScale || band_scale == 0.0)
            band_scale = 1.0;
        if (band_scale < 0.0) {
            fprintf(stderr, "Error: negative DEM band scales are not supported\n");
            GDALClose(ds);
            return false;
        }
        info->band_scale = band_scale;
    }

    GDALClose(ds);
    return true;
}

static bool _parse_gdalinfo_mm(const char *dem_path, double *min_val, double *max_val) {
    GDALDatasetH ds = GDALOpen(dem_path, GA_ReadOnly);
    if (!ds) {
        fprintf(stderr, "Error: GDALOpen failed for: %s\n", dem_path);
        return false;
    }

    GDALRasterBandH band = GDALGetRasterBand(ds, 1);
    CPLErr          err  = GDALComputeRasterStatistics(band, FALSE, min_val, max_val, NULL, NULL, NULL, NULL);
    GDALClose(ds);

    if (err != CE_None) {
        fprintf(stderr, "Error: GDALComputeRasterStatistics failed\n");
        return false;
    }
    return true;
}

static uint16_t _scale_raw_to_u16(double value, double min_h, double max_h) {
    if (max_h <= min_h)
        return 0;
    double t = (value - min_h) / (max_h - min_h);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return (uint16_t)lrint(t * 65535.0);
}

static bool _write_height_tile(const char *input, const char *output, uint32_t src_x, uint32_t src_y, uint32_t w, uint32_t h, uint32_t tile_size, double min_h, double max_h) {
    if (tile_size == 0 || w == 0 || h == 0 ||
        tile_size > (uint32_t)INT32_MAX || w > (uint32_t)INT32_MAX || h > (uint32_t)INT32_MAX) {
        fprintf(stderr, "Error: tile dimensions are too large for GDALRasterIO\n");
        return false;
    }
    if ((size_t)tile_size > ((size_t)-1) / (size_t)tile_size ||
        (size_t)w > ((size_t)-1) / (size_t)h / sizeof(double)) {
        fprintf(stderr, "Error: tile buffer size overflow\n");
        return false;
    }

    GDALDatasetH src_ds = GDALOpen(input, GA_ReadOnly);
    if (!src_ds) {
        fprintf(stderr, "Error: GDALOpen failed for: %s\n", input);
        return false;
    }

    GDALRasterBandH band = GDALGetRasterBand(src_ds, 1);
    if (!band) {
        fprintf(stderr, "Error: DEM has no raster band\n");
        GDALClose(src_ds);
        return false;
    }

    size_t tile_pixel_count = (size_t)tile_size * (size_t)tile_size;
    uint16_t *tile_data = (uint16_t *)calloc(tile_pixel_count, sizeof(uint16_t));
    double *raw_data = (double *)malloc((size_t)w * (size_t)h * sizeof(double));
    if (!tile_data || !raw_data) {
        fprintf(stderr, "Error: failed to allocate tile buffers\n");
        free(tile_data);
        free(raw_data);
        GDALClose(src_ds);
        return false;
    }

    uint16_t fill_value = _scale_raw_to_u16(min_h, min_h, max_h);
    for (size_t i = 0; i < tile_pixel_count; i++)
        tile_data[i] = fill_value;

    CPLErr read_err = GDALRasterIO(band, GF_Read, (int)src_x, (int)src_y, (int)w, (int)h,
                                   raw_data, (int)w, (int)h, GDT_Float64, 0, 0);
    if (read_err != CE_None) {
        fprintf(stderr, "Error: GDALRasterIO failed for tile at (%u, %u)\n", src_x, src_y);
        free(tile_data);
        free(raw_data);
        GDALClose(src_ds);
        return false;
    }

    int has_nodata = 0;
    double nodata = GDALGetRasterNoDataValue(band, &has_nodata);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            double value = raw_data[x + y * w];
            if ((has_nodata && value == nodata) || isnan(value))
                value = min_h;
            tile_data[x + y * tile_size] = _scale_raw_to_u16(value, min_h, max_h);
        }
    }
    free(raw_data);

    GDALDriverH mem_driver = GDALGetDriverByName("MEM");
    GDALDriverH png_driver = GDALGetDriverByName("PNG");
    if (!mem_driver || !png_driver) {
        fprintf(stderr, "Error: GDAL MEM/PNG drivers are unavailable\n");
        free(tile_data);
        GDALClose(src_ds);
        return false;
    }

    GDALDatasetH mem_ds = GDALCreate(mem_driver, "", (int)tile_size, (int)tile_size, 1, GDT_UInt16, NULL);
    if (!mem_ds) {
        fprintf(stderr, "Error: failed to create temporary tile dataset\n");
        free(tile_data);
        GDALClose(src_ds);
        return false;
    }

    GDALRasterBandH out_band = GDALGetRasterBand(mem_ds, 1);
    CPLErr write_err = GDALRasterIO(out_band, GF_Write, 0, 0, (int)tile_size, (int)tile_size,
                                    tile_data, (int)tile_size, (int)tile_size, GDT_UInt16, 0, 0);
    free(tile_data);
    if (write_err != CE_None) {
        fprintf(stderr, "Error: failed to write temporary tile dataset\n");
        GDALClose(mem_ds);
        GDALClose(src_ds);
        return false;
    }

    GDALDatasetH dst_ds = GDALCreateCopy(png_driver, output, mem_ds, FALSE, NULL, NULL, NULL);
    GDALClose(mem_ds);
    GDALClose(src_ds);
    if (!dst_ds) {
        fprintf(stderr, "Error: GDALCreateCopy failed for tile at (%u, %u)\n", src_x, src_y);
        return false;
    }

    GDALClose(dst_ds);
    return true;
}
