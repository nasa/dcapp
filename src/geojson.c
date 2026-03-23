#include "geojson.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/stb_sb.h"

#include "pl_json.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DC_GEOJSON_TAG "GeoJSON"

//-----------------------------------------------------------------------------
// internal types
//-----------------------------------------------------------------------------

typedef struct DcGeojsonData {
    DcGeojsonFeature *sb_features;
    bool              used;
} DcGeojsonData;

static DcGeojsonData *_sb_datas = NULL;

//-----------------------------------------------------------------------------
// internal helpers
//-----------------------------------------------------------------------------

static void
_ensure_init(void)
{
    if (!_sb_datas) {
        DcGeojsonData reserved = {0};
        sbpush(_sb_datas, reserved);
    }
}

static DcGeojsonData *
_get(DcGeojsonHandle handle)
{
    if (handle.index == DC_GEOJSON_UNDEFINED || handle.index >= (uint8_t)sbcount(_sb_datas) || !_sb_datas[handle.index].used)
        return NULL;
    return &_sb_datas[handle.index];
}

static uint32_t
_json_array_count(plJsonObject *array)
{
    uint32_t count = 0;
    while (pl_json_member_by_index(array, count) != NULL)
        count++;
    return count;
}

// pl_json_member_by_name uses strncmp with the child's name length,
// so "stroke-opacity" matches "stroke". This does an exact match.
static plJsonObject *
_json_member(plJsonObject *json, const char *name)
{
    if (!json) return NULL;
    for (uint32_t i = 0; ; i++) {
        plJsonObject *child = pl_json_member_by_index(json, i);
        if (!child) break;
        const char *child_name = pl_json_get_name(child);
        if (child_name && strcmp(name, child_name) == 0)
            return child;
    }
    return NULL;
}

static DcGeojsonFeature *
_push_feature(DcGeojsonData *gj)
{
    DcGeojsonFeature feat;
    memset(&feat, 0, sizeof(DcGeojsonFeature));
    sbpush(gj->sb_features, feat);
    return &gj->sb_features[sbcount(gj->sb_features) - 1];
}

static bool
_parse_hex_color(const char *hex, DcGeojsonColor *color_out)
{
    if (!hex || hex[0] != '#') return false;

    unsigned int r, g, b, a = 255;
    int len = (int)strlen(hex);

    if (len == 7) {
        if (sscanf(hex, "#%02x%02x%02x", &r, &g, &b) != 3)
            return false;
    } else if (len == 9) {
        if (sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a) != 4)
            return false;
    } else {
        return false;
    }

    color_out->r = (float)r / 255.0f;
    color_out->g = (float)g / 255.0f;
    color_out->b = (float)b / 255.0f;
    color_out->a = (float)a / 255.0f;
    color_out->has_value = true;
    return true;
}

static void
_parse_style_properties(plJsonObject *properties, DcGeojsonStyle *style)
{
    if (!properties) return;

    // stroke color
    char stroke_buf[32] = {0};
    pl_json_string_member(properties, "stroke", stroke_buf, sizeof(stroke_buf));
    if (stroke_buf[0] != '\0') {
        if (_parse_hex_color(stroke_buf, &style->stroke)) {
            double opacity = pl_json_double_member(properties, "stroke-opacity", 1.0);
            style->stroke.a *= (float)opacity;
        }
    }

    // stroke-width
    if (pl_json_member(properties, "stroke-width")) {
        style->stroke_width = (float)pl_json_double_member(properties, "stroke-width", 1.0);
        style->has_stroke_width = true;
    }

    // fill color
    char fill_buf[32] = {0};
    pl_json_string_member(properties, "fill", fill_buf, sizeof(fill_buf));
    if (fill_buf[0] != '\0') {
        if (_parse_hex_color(fill_buf, &style->fill)) {
            double opacity = pl_json_double_member(properties, "fill-opacity", 0.5);
            style->fill.a *= (float)opacity;
        }
    }
}

//-----------------------------------------------------------------------------
// coordinate parsing
//-----------------------------------------------------------------------------

static bool
_parse_coord_array(plJsonObject *coord_array, uint32_t coord_count, DcGeojsonCoordArray *out)
{
    out->count = coord_count;
    out->positions = calloc(coord_count, sizeof(DcGeojsonPosition));
    if (!out->positions) {
        DC_LOG_ERROR(DC_GEOJSON_TAG, "allocation failed for %u positions", coord_count);
        return false;
    }

    for (uint32_t i = 0; i < coord_count; i++) {
        plJsonObject *coord = pl_json_member_by_index(coord_array, i);

        uint32_t dim_count = 0;
        double dims[4] = {0};
        pl_json_as_double_array(coord, dims, &dim_count);

        if (dim_count < 2) {
            free(out->positions);
            out->positions = NULL;
            out->count = 0;
            return false;
        }

        out->positions[i].lon     = dims[0];
        out->positions[i].lat     = dims[1];
        out->positions[i].alt     = dim_count >= 3 ? dims[2] : 0.0;
        out->positions[i].has_alt = dim_count >= 3;
    }
    return true;
}

static bool
_parse_point_coords(plJsonObject *geom, DcGeojsonPosition *point_out)
{
    double dims[4] = {0};
    uint32_t dim_count = 0;
    pl_json_double_array_member(geom, "coordinates", dims, &dim_count);
    if (dim_count < 2) return false;

    point_out->lon     = dims[0];
    point_out->lat     = dims[1];
    point_out->alt     = dim_count >= 3 ? dims[2] : 0.0;
    point_out->has_alt = dim_count >= 3;
    return true;
}

//-----------------------------------------------------------------------------
// feature cleanup
//-----------------------------------------------------------------------------

static void
_free_feature(DcGeojsonFeature *feat)
{
    switch (feat->type) {
        case DC_GEOJSON_FEATURE_UNDEFINED:
        case DC_GEOJSON_FEATURE_POINT:
            break;

        case DC_GEOJSON_FEATURE_MULTI_POINT:
            free(feat->geom.multi_point.positions);
            break;

        case DC_GEOJSON_FEATURE_LINE_STRING:
            free(feat->geom.line_string.positions);
            break;

        case DC_GEOJSON_FEATURE_MULTI_LINE_STRING:
            for (uint32_t i = 0; i < feat->geom.multi_line_string.count; i++)
                free(feat->geom.multi_line_string.line_strings[i].positions);
            free(feat->geom.multi_line_string.line_strings);
            break;

        case DC_GEOJSON_FEATURE_POLYGON:
            for (uint32_t i = 0; i < feat->geom.polygon.ring_count; i++)
                free(feat->geom.polygon.rings[i].positions);
            free(feat->geom.polygon.rings);
            break;

        case DC_GEOJSON_FEATURE_MULTI_POLYGON:
            for (uint32_t i = 0; i < feat->geom.multi_polygon.count; i++) {
                for (uint32_t j = 0; j < feat->geom.multi_polygon.polygons[i].ring_count; j++)
                    free(feat->geom.multi_polygon.polygons[i].rings[j].positions);
                free(feat->geom.multi_polygon.polygons[i].rings);
            }
            free(feat->geom.multi_polygon.polygons);
            break;

        case DC_GEOJSON_FEATURE_GEOMETRY_COLLECTION:
            for (uint32_t i = 0; i < feat->geom.geometry_collection.count; i++)
                _free_feature(&feat->geom.geometry_collection.features[i]);
            free(feat->geom.geometry_collection.features);
            break;
    }
}

//-----------------------------------------------------------------------------
// geometry parsing
//-----------------------------------------------------------------------------

static bool _parse_geometry(DcGeojsonFeature *feat, plJsonObject *geom);

static bool
_parse_geometry(DcGeojsonFeature *feat, plJsonObject *geom)
{
    char type_buf[64] = {0};
    pl_json_string_member(geom, "type", type_buf, sizeof(type_buf));

    if (strcmp(type_buf, "Point") == 0) {
        feat->type = DC_GEOJSON_FEATURE_POINT;
        return _parse_point_coords(geom, &feat->geom.point.position);
    }
    else if (strcmp(type_buf, "MultiPoint") == 0) {
        uint32_t coord_count = 0;
        plJsonObject *coords = pl_json_array_member(geom, "coordinates", &coord_count);
        if (!coords) return false;

        feat->type = DC_GEOJSON_FEATURE_MULTI_POINT;
        return _parse_coord_array(coords, coord_count, &feat->geom.multi_point);
    }
    else if (strcmp(type_buf, "LineString") == 0) {
        uint32_t coord_count = 0;
        plJsonObject *coords = pl_json_array_member(geom, "coordinates", &coord_count);
        if (!coords) return false;

        feat->type = DC_GEOJSON_FEATURE_LINE_STRING;
        return _parse_coord_array(coords, coord_count, &feat->geom.line_string);
    }
    else if (strcmp(type_buf, "MultiLineString") == 0) {
        uint32_t line_count = 0;
        plJsonObject *coords = pl_json_array_member(geom, "coordinates", &line_count);
        if (!coords) return false;

        feat->type = DC_GEOJSON_FEATURE_MULTI_LINE_STRING;
        feat->geom.multi_line_string.line_strings = calloc(line_count, sizeof(DcGeojsonLineString));
        if (!feat->geom.multi_line_string.line_strings) {
            DC_LOG_ERROR(DC_GEOJSON_TAG, "allocation failed for %u line strings", line_count);
            return false;
        }
        feat->geom.multi_line_string.count = line_count;

        for (uint32_t i = 0; i < line_count; i++) {
            plJsonObject *line_coords = pl_json_member_by_index(coords, i);
            uint32_t coord_count = _json_array_count(line_coords);
            if (!_parse_coord_array(line_coords, coord_count, &feat->geom.multi_line_string.line_strings[i]))
                return false;
        }
        return true;
    }
    else if (strcmp(type_buf, "Polygon") == 0) {
        uint32_t ring_count = 0;
        plJsonObject *coords = pl_json_array_member(geom, "coordinates", &ring_count);
        if (!coords) return false;

        feat->type = DC_GEOJSON_FEATURE_POLYGON;
        feat->geom.polygon.rings = calloc(ring_count, sizeof(DcGeojsonCoordArray));
        if (!feat->geom.polygon.rings) {
            DC_LOG_ERROR(DC_GEOJSON_TAG, "allocation failed for %u rings", ring_count);
            return false;
        }
        feat->geom.polygon.ring_count = ring_count;

        for (uint32_t i = 0; i < ring_count; i++) {
            plJsonObject *ring = pl_json_member_by_index(coords, i);
            uint32_t coord_count = _json_array_count(ring);
            if (!_parse_coord_array(ring, coord_count, &feat->geom.polygon.rings[i]))
                return false;
        }
        return true;
    }
    else if (strcmp(type_buf, "MultiPolygon") == 0) {
        uint32_t poly_count = 0;
        plJsonObject *coords = pl_json_array_member(geom, "coordinates", &poly_count);
        if (!coords) return false;

        feat->type = DC_GEOJSON_FEATURE_MULTI_POLYGON;
        feat->geom.multi_polygon.polygons = calloc(poly_count, sizeof(DcGeojsonPolygon));
        if (!feat->geom.multi_polygon.polygons) {
            DC_LOG_ERROR(DC_GEOJSON_TAG, "allocation failed for %u polygons", poly_count);
            return false;
        }
        feat->geom.multi_polygon.count = poly_count;

        for (uint32_t i = 0; i < poly_count; i++) {
            plJsonObject *poly_coords = pl_json_member_by_index(coords, i);
            uint32_t ring_count = _json_array_count(poly_coords);

            feat->geom.multi_polygon.polygons[i].rings = calloc(ring_count, sizeof(DcGeojsonCoordArray));
            if (!feat->geom.multi_polygon.polygons[i].rings) {
                DC_LOG_ERROR(DC_GEOJSON_TAG, "allocation failed for %u rings", ring_count);
                return false;
            }
            feat->geom.multi_polygon.polygons[i].ring_count = ring_count;

            for (uint32_t j = 0; j < ring_count; j++) {
                plJsonObject *ring = pl_json_member_by_index(poly_coords, j);
                uint32_t coord_count = _json_array_count(ring);
                if (!_parse_coord_array(ring, coord_count, &feat->geom.multi_polygon.polygons[i].rings[j]))
                    return false;
            }
        }
        return true;
    }
    else if (strcmp(type_buf, "GeometryCollection") == 0) {
        uint32_t geom_count = 0;
        plJsonObject *geometries = pl_json_array_member(geom, "geometries", &geom_count);
        if (!geometries) return false;

        feat->type = DC_GEOJSON_FEATURE_GEOMETRY_COLLECTION;
        feat->geom.geometry_collection.features = calloc(geom_count, sizeof(DcGeojsonFeature));
        if (!feat->geom.geometry_collection.features) {
            DC_LOG_ERROR(DC_GEOJSON_TAG, "allocation failed for %u geometries", geom_count);
            return false;
        }
        feat->geom.geometry_collection.count = geom_count;

        for (uint32_t i = 0; i < geom_count; i++) {
            plJsonObject *child = pl_json_member_by_index(geometries, i);
            if (!_parse_geometry(&feat->geom.geometry_collection.features[i], child))
                return false;
        }
        return true;
    }

    DC_LOG_WARN(DC_GEOJSON_TAG, "unsupported geometry type: %s", type_buf);
    return true;
}

//-----------------------------------------------------------------------------
// public api
//-----------------------------------------------------------------------------

DcGeojsonHandle
dc_geojson_load(const char *filepath)
{
    DcGeojsonHandle handle = { .index = DC_GEOJSON_UNDEFINED };
    _ensure_init();

    // find free slot (0 is reserved as invalid)
    uint8_t slot = DC_GEOJSON_UNDEFINED;
    for (int i = 1; i < sbcount(_sb_datas); i++) {
        if (!_sb_datas[i].used) {
            slot = (uint8_t)i;
            break;
        }
    }

    // no free slot found, append a new one
    if (slot == DC_GEOJSON_UNDEFINED) {
        if (sbcount(_sb_datas) >= 255) {
            DC_LOG_ERROR(DC_GEOJSON_TAG, "no free slots");
            return handle;
        }
        DcGeojsonData empty = {0};
        sbpush(_sb_datas, empty);
        slot = (uint8_t)(sbcount(_sb_datas) - 1);
    }

    char *json_str = dc_utils_load_text_file(filepath);
    if (!json_str) {
        DC_LOG_ERROR(DC_GEOJSON_TAG, "failed to load file: %s", filepath);
        return handle;
    }

    plJsonObject *root = NULL;
    if (!pl_load_json(json_str, &root)) {
        DC_LOG_ERROR(DC_GEOJSON_TAG, "failed to parse JSON: %s", filepath);
        free(json_str);
        return handle;
    }

    DcGeojsonData *gj = &_sb_datas[slot];
    memset(gj, 0, sizeof(DcGeojsonData));
    gj->used = true;

    char type_buf[64] = {0};
    pl_json_string_member(root, "type", type_buf, sizeof(type_buf));

    bool ok = true;

    if (strcmp(type_buf, "FeatureCollection") == 0) {
        uint32_t fc = 0;
        plJsonObject *feat_array = pl_json_array_member(root, "features", &fc);
        if (!feat_array) {
            ok = false;
        } else {
            for (uint32_t i = 0; i < fc; i++) {
                plJsonObject *feature = pl_json_member_by_index(feat_array, i);
                plJsonObject *geom = pl_json_member(feature, "geometry");
                plJsonObject *props = pl_json_member(feature, "properties");
                if (geom) {
                    DcGeojsonFeature *feat = _push_feature(gj);
                    _parse_style_properties(props, &feat->style);
                    if (!_parse_geometry(feat, geom)) { ok = false; break; }
                }
            }
        }
    }
    else if (strcmp(type_buf, "Feature") == 0) {
        plJsonObject *geom = pl_json_member(root, "geometry");
        plJsonObject *props = pl_json_member(root, "properties");
        if (geom) {
            DcGeojsonFeature *feat = _push_feature(gj);
            _parse_style_properties(props, &feat->style);
            ok = _parse_geometry(feat, geom);
        } else {
            ok = false;
        }
    }
    else {
        // bare geometry
        DcGeojsonFeature *feat = _push_feature(gj);
        ok = _parse_geometry(feat, root);
    }

    pl_unload_json(&root);
    free(json_str);

    if (!ok) {
        handle.index = slot;
        dc_geojson_free(handle);
        handle.index = DC_GEOJSON_UNDEFINED;
        return handle;
    }

    DC_LOG_INFO(DC_GEOJSON_TAG, "loaded %s: %d features", filepath, sbcount(gj->sb_features));
    handle.index = slot;
    return handle;
}

void
dc_geojson_free(DcGeojsonHandle handle)
{
    DcGeojsonData *gj = _get(handle);
    if (!gj) return;

    for (int i = 0; i < sbcount(gj->sb_features); i++)
        _free_feature(&gj->sb_features[i]);
    sbfree(gj->sb_features);
    memset(gj, 0, sizeof(DcGeojsonData));
}

uint32_t
dc_geojson_feature_count(DcGeojsonHandle handle)
{
    DcGeojsonData *gj = _get(handle);
    return gj ? (uint32_t)sbcount(gj->sb_features) : 0;
}

const DcGeojsonFeature *
dc_geojson_feature(DcGeojsonHandle handle, uint32_t index)
{
    DcGeojsonData *gj = _get(handle);
    if (!gj || index >= (uint32_t)sbcount(gj->sb_features)) return NULL;
    return &gj->sb_features[index];
}
