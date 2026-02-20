/*
   dcapp-planet-chunkgen.c

   Pilotlight app that converts a GeoTIFF DEM into .chu chunk files
   for the planet rendering pipeline.

   Usage:
     pilot_light -a dcapp-planet-chunkgen <input_dem> <output_dir> --radius N [options]
*/

/*
Index of this file:
// [SECTION] includes
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
#include "pl.h"
#include "../src/utils/file.h"
#include "pl_planet_processor_ext.h"

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

static bool  _parse_gdalinfo(const char *dem_path, uint32_t *width, uint32_t *height, double *pixel_scale, double *origin_x, double *origin_y);
static bool  _parse_gdalinfo_mm(const char *dem_path, double *min_val, double *max_val);
static bool  _run_gdal_translate(const char *input, const char *output, uint32_t src_x, uint32_t src_y, uint32_t w, uint32_t h, double min_h, double max_h);
static void  _compute_tile_latlon(double origin_x, double origin_y, double pixel_scale, uint32_t col, uint32_t row, uint32_t tile_size, double radius, float *lat_deg, float *lon_deg);
static void  _show_help(void);
static char *_get_stem(const char *path, char *buf, size_t buf_size);

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
    extension_registry->load("pl_platform_ext", NULL, NULL, false);
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
    float       max_base_error   = 15.0f;
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

    if (radius <= 0.0) {
        fprintf(stderr, "Error: --radius is required and must be positive\n");
        io->bRunning = false;
        return NULL;
    }

    if (!dc_utils_file_exists(input_dem)) {
        fprintf(stderr, "Error: input file not found: %s\n", input_dem);
        io->bRunning = false;
        return NULL;
    }

    GDALAllRegister();

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

    uint32_t raster_w = 0, raster_h = 0;
    double   pixel_scale = 0.0, origin_x = 0.0, origin_y = 0.0;

    if (!_parse_gdalinfo(input_dem, &raster_w, &raster_h, &pixel_scale, &origin_x, &origin_y)) {
        io->bRunning = false;
        return NULL;
    }

    printf("Raster size: %u x %u\n", raster_w, raster_h);
    printf("Radius: %.1f m\n", radius);
    printf("Pixel scale: %.6f m\n", pixel_scale);
    printf("Origin: (%.6f, %.6f)\n", origin_x, origin_y);

    // auto-detect meters per pixel from DEM pixel scale
    if (meters_per_pixel <= 0.0) {
        if (pixel_scale > 0.0) {
            meters_per_pixel = pixel_scale;
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
        if (isnan(min_height))
            min_height = detected_min;
        if (isnan(max_height))
            max_height = detected_max;
    }

    printf("Elevation range: %.2f to %.2f m\n", min_height, max_height);

    //---- compute tile grid ----

    uint32_t cols       = (raster_w + tile_size - 1) / tile_size;
    uint32_t rows       = (raster_h + tile_size - 1) / tile_size;
    uint32_t tile_count = cols * rows;

    printf("Tile size: %u px\n", tile_size);
    printf("Grid: %u x %u (%u tiles)\n", cols, rows, tile_count);
    printf("Tree depth: %d\n", tree_depth);
    printf("Max base error: %.2f\n", max_base_error);
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
        .fRadius          = (float)radius,
        .fMetersPerPixel  = (float)meters_per_pixel,
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
            tiles[idx].fMaxHeight    = (float)max_height;
            tiles[idx].fMinHeight    = (float)min_height;
            tiles[idx].fMaxBaseError = max_base_error;

            // compute lat/lon for tile center
            float lat = 0.0f, lon = 0.0f;
            _compute_tile_latlon(origin_x, origin_y, pixel_scale,
                                 col, row, tile_size, radius, &lat, &lon);
            tiles[idx].fLatitude  = lat;
            tiles[idx].fLongitude = lon;

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

    plJsonObject *tile_array = pl_json_add_member_array(root, "tiles", tile_count);
    for (uint32_t i = 0; i < tile_count; i++) {
        plJsonObject *tile_obj = pl_json_member_by_index(tile_array, i);

        const char *filename = strrchr(tiles[i].acOutputFile, '/');
        if (filename)
            filename++;
        else
            filename = tiles[i].acOutputFile;

        pl_json_add_float_member(tile_obj, "lat", tiles[i].fLatitude);
        pl_json_add_float_member(tile_obj, "lon", tiles[i].fLongitude);
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
    }
    free(json_buf);

    //---- tile DEM with GDAL ----

    printf("\n[Step 2/%d] Tiling DEM into %u PNGs...\n", 3, tile_count);

    for (uint32_t t = 0; t < tile_count; t++) {
        uint32_t col = t % cols;
        uint32_t row = t / cols;
        uint32_t sx  = col * tile_size;
        uint32_t sy  = row * tile_size;
        uint32_t w   = tile_size;
        uint32_t h   = tile_size;
        if (sx + w > raster_w)
            w = raster_w - sx;
        if (sy + h > raster_h)
            h = raster_h - sy;

        char tile_path[2048];
        snprintf(tile_path, sizeof(tile_path), "%s/%s_%u_%u.png", output_dir, prefix, col, row);

        printf("  [%u/%u] %s_%u_%u.png\n", t + 1, tile_count, prefix, col, row);

        if (!_run_gdal_translate(input_dem, tile_path, sx, sy, w, h, min_height, max_height)) {
            io->bRunning = false;
            return NULL;
        }
    }

    //---- process chunks ----

    printf("\n[Step 3/%d] Processing chunks...\n", 3);

    _ext_planet_processor->process(&planet_info);

    //---- cleanup ----

    if (!keep_tiles) {
        printf("Cleaning up intermediate PNGs...\n");
        for (uint32_t i = 0; i < tile_count; i++) {
            remove(tiles[i].acHeightMapFile);
            // also remove .aux.xml sidecar files that GDAL creates
            char aux[512];
            snprintf(aux, sizeof(aux), "%s.aux.xml", tiles[i].acHeightMapFile);
            remove(aux);
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
    printf("  dcapp-planet-chunkgen <input_dem> <output_dir> --radius N [options]\n\n");
    printf("Required:\n");
    printf("  --radius N             Planet radius in meters\n\n");
    printf("Options:\n");
    printf("  --tile-size N          Tile dimensions in pixels (default: 4096)\n");
    printf("  --min-height N         Min elevation in meters (default: auto-detect)\n");
    printf("  --max-height N         Max elevation in meters (default: auto-detect)\n");
    printf("  --meters-per-pixel N   Meters per pixel (default: auto-detect from DEM)\n");
    printf("  --tree-depth N         CDLOD quadtree depth (default: 6)\n");
    printf("  --max-base-error N     LOD error threshold (default: 15.0)\n");
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

static bool _parse_gdalinfo(const char *dem_path, uint32_t *width, uint32_t *height, double *pixel_scale, double *origin_x, double *origin_y) {
    GDALDatasetH ds = GDALOpen(dem_path, GA_ReadOnly);
    if (!ds) {
        fprintf(stderr, "Error: GDALOpen failed for: %s\n", dem_path);
        return false;
    }

    *width  = (uint32_t)GDALGetRasterXSize(ds);
    *height = (uint32_t)GDALGetRasterYSize(ds);

    double gt[6] = {0};
    if (GDALGetGeoTransform(ds, gt) == CE_None) {
        *pixel_scale = fabs(gt[1]);
        *origin_x    = gt[0];
        *origin_y    = gt[3] + (double)(*height) * gt[5]; // lower-left Y
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
    CPLErr err = GDALComputeRasterStatistics(band, FALSE, min_val, max_val, NULL, NULL, NULL, NULL);
    GDALClose(ds);

    if (err != CE_None) {
        fprintf(stderr, "Error: GDALComputeRasterStatistics failed\n");
        return false;
    }
    return true;
}

static bool _run_gdal_translate(const char *input, const char *output, uint32_t src_x, uint32_t src_y, uint32_t w, uint32_t h, double min_h, double max_h) {
    GDALDatasetH src_ds = GDALOpen(input, GA_ReadOnly);
    if (!src_ds) {
        fprintf(stderr, "Error: GDALOpen failed for: %s\n", input);
        return false;
    }

    char sx_str[32],  sy_str[32],  w_str[32],  h_str[32];
    char min_str[64], max_str[64];
    snprintf(sx_str,  sizeof(sx_str),  "%u",  src_x);
    snprintf(sy_str,  sizeof(sy_str),  "%u",  src_y);
    snprintf(w_str,   sizeof(w_str),   "%u",  w);
    snprintf(h_str,   sizeof(h_str),   "%u",  h);
    snprintf(min_str, sizeof(min_str), "%f",  min_h);
    snprintf(max_str, sizeof(max_str), "%f",  max_h);

    char *args[] = {
        "-r",      "cubic",
        "-of",     "PNG",
        "-srcwin", sx_str, sy_str, w_str, h_str,
        "-ot",     "UInt16",
        "-scale",  min_str, max_str, "0", "65535",
        NULL
    };

    GDALTranslateOptions *opts    = GDALTranslateOptionsNew(args, NULL);
    int                   bError  = 0;
    GDALDatasetH          dst_ds  = GDALTranslate(output, src_ds, opts, &bError);
    GDALTranslateOptionsFree(opts);
    GDALClose(src_ds);

    if (!dst_ds || bError) {
        fprintf(stderr, "Error: GDALTranslate failed for tile at (%u, %u)\n", src_x, src_y);
        if (dst_ds) GDALClose(dst_ds);
        return false;
    }

    GDALClose(dst_ds);
    return true;
}

//-----------------------------------------------------------------------------
// [SECTION] stereographic projection
//-----------------------------------------------------------------------------

static void _compute_tile_latlon(double origin_x, double origin_y, double pixel_scale, uint32_t col, uint32_t row, uint32_t tile_size, double radius, float *lat_deg, float *lon_deg) {
    // model coordinates of tile center
    double model_x = origin_x + ((double)col * tile_size + tile_size * 0.5) * pixel_scale;
    double model_y = origin_y + ((double)row * tile_size + tile_size * 0.5) * pixel_scale;

    // inverse south-pole stereographic (matching dcapp-terrain.c:292-300)
    float x         = (float)model_x;
    float y         = (float)model_y;
    float longitude = atan2f(x, y);
    float r         = x / sinf(longitude);
    float latitude  = (float)(M_PI / 2.0) - 2.0f * atanf(r / (2.0f * (float)radius));

    *lon_deg = longitude * (180.0f / (float)M_PI);
    *lat_deg = latitude * (180.0f / (float)M_PI);
}
