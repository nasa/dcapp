#ifndef DC_GEOJSON_H
#define DC_GEOJSON_H

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// types
//-----------------------------------------------------------------------------

#define DC_GEOJSON_UNDEFINED 0

typedef struct DcGeojsonHandle {
    uint8_t index;
} DcGeojsonHandle;

typedef enum {
    DC_GEOJSON_FEATURE_UNDEFINED,
    DC_GEOJSON_FEATURE_GEOMETRY_COLLECTION,
    DC_GEOJSON_FEATURE_LINE_STRING,
    DC_GEOJSON_FEATURE_MULTI_LINE_STRING,
    DC_GEOJSON_FEATURE_MULTI_POINT,
    DC_GEOJSON_FEATURE_MULTI_POLYGON,
    DC_GEOJSON_FEATURE_POINT,
    DC_GEOJSON_FEATURE_POLYGON,
} DcGeojsonFeatureType;

typedef struct DcGeojsonPosition {
    double lon;
    double lat;
    double alt;
    bool   has_alt;
} DcGeojsonPosition;

typedef struct DcGeojsonColor {
    float r, g, b, a;
    bool  has_value;
} DcGeojsonColor;

typedef struct DcGeojsonStyle {
    DcGeojsonColor stroke;
    DcGeojsonColor fill;
    float          stroke_width;
    bool           has_stroke_width;
} DcGeojsonStyle;

typedef struct DcGeojsonCoordArray {
    DcGeojsonPosition *positions;
    uint32_t           count;
} DcGeojsonCoordArray;

typedef DcGeojsonCoordArray DcGeojsonLineString;

typedef struct DcGeojsonPolygon {
    DcGeojsonCoordArray *rings;
    uint32_t             ring_count;
} DcGeojsonPolygon;

typedef struct DcGeojsonFeature DcGeojsonFeature;

struct DcGeojsonFeature {
    DcGeojsonFeatureType type;
    DcGeojsonStyle       style;
    union {
        struct {
            DcGeojsonPosition position;
        } point;
        DcGeojsonCoordArray multi_point;
        DcGeojsonLineString line_string;
        struct {
            DcGeojsonLineString *line_strings;
            uint32_t             count;
        } multi_line_string;
        DcGeojsonPolygon polygon;
        struct {
            DcGeojsonPolygon *polygons;
            uint32_t          count;
        } multi_polygon;
        struct {
            DcGeojsonFeature *features;
            uint32_t          count;
        } geometry_collection;
    } geom;
};

//-----------------------------------------------------------------------------
// api
//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

DcGeojsonHandle         dc_geojson_load(const char *filepath);
void                    dc_geojson_free(DcGeojsonHandle geojson);
uint32_t                dc_geojson_feature_count(DcGeojsonHandle geojson);
const DcGeojsonFeature *dc_geojson_feature(DcGeojsonHandle geojson, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif
