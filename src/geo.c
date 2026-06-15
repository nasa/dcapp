#include "geo.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// For pl_norm_vec3 and other vector operations
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"

// Factory functions
DcGeoCrsGeodetic dc_geo_create_crs_geodetic(double planet_radius) {
    return (DcGeoCrsGeodetic){.planet_radius = planet_radius};
}

DcGeoCrsCartesian dc_geo_create_crs_cartesian(double planet_radius) {
    return (DcGeoCrsCartesian){.planet_radius = planet_radius};
}

DcGeoCrsPolarStereo dc_geo_create_crs_polar_stereographic(double planet_radius, double lat_origin, double lon_origin) {
    return (DcGeoCrsPolarStereo){
        .planet_radius = planet_radius,
        .lat_origin = lat_origin,
        .lon_origin = lon_origin
    };
}

// Coordinate conversions
void dc_geo_geodetic_to_cartesian(const DcGeoCrsGeodetic *from, const DcGeoCrsCartesian *to, const plVec3 *in, plVec3 *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        float lat_rad = in[i].x * (float)M_PI / 180.0f;
        float lon_rad = in[i].y * (float)M_PI / 180.0f;
        float r = (float)(planet_radius + in[i].z);
        out[i].x = r * cosf(lat_rad) * sinf(lon_rad);
        out[i].y = r * sinf(lat_rad);
        out[i].z = r * cosf(lat_rad) * cosf(lon_rad);
    }
}

void dc_geo_geodetic_to_cartesian_d(const DcGeoCrsGeodetic *from, const DcGeoCrsCartesian *to, const plVec3d *in, plVec3d *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        double lat_rad = in[i].x * M_PI / 180.0;
        double lon_rad = in[i].y * M_PI / 180.0;
        double r = planet_radius + in[i].z;
        out[i].x = r * cos(lat_rad) * sin(lon_rad);
        out[i].y = r * sin(lat_rad);
        out[i].z = r * cos(lat_rad) * cos(lon_rad);
    }
}

void dc_geo_cartesian_to_geodetic(const DcGeoCrsCartesian *from, const DcGeoCrsGeodetic *to, const plVec3 *in, plVec3 *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        float x = in[i].x;
        float y = in[i].y;
        float z = in[i].z;
        float r = sqrtf(x * x + y * y + z * z);
        if (r > 0.0f) {
            out[i].x = asinf(y / r) * 180.0f / (float)M_PI;
            out[i].y = atan2f(x, z) * 180.0f / (float)M_PI;
            out[i].z = r - (float)planet_radius;
        } else {
            out[i].x = 0.0f;
            out[i].y = 0.0f;
            out[i].z = -(float)planet_radius;
        }
    }
}

void dc_geo_cartesian_to_geodetic_d(const DcGeoCrsCartesian *from, const DcGeoCrsGeodetic *to, const plVec3d *in, plVec3d *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        double x = in[i].x;
        double y = in[i].y;
        double z = in[i].z;
        double r = sqrt(x * x + y * y + z * z);
        if (r > 0.0) {
            out[i].x = asin(y / r) * 180.0 / M_PI;
            out[i].y = atan2(x, z) * 180.0 / M_PI;
            out[i].z = r - planet_radius;
        } else {
            out[i].x = 0.0;
            out[i].y = 0.0;
            out[i].z = -planet_radius;
        }
    }
}

void dc_geo_geodetic_to_polar_stereo(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3 *in, plVec2 *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        float lat_rad = in[i].x * (float)M_PI / 180.0f;
        float lon_rad = in[i].y * (float)M_PI / 180.0f;
        float rho = 2.0f * (float)planet_radius * tanf((float)M_PI / 4.0f + 0.5f * lat_rad);
        out[i].x = rho * sinf(lon_rad);
        out[i].y = -rho * cosf(lon_rad);
    }
}

void dc_geo_geodetic_to_polar_stereo_d(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3d *in, plVec2d *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        double lat_rad = in[i].x * M_PI / 180.0;
        double lon_rad = in[i].y * M_PI / 180.0;
        double rho = 2.0 * planet_radius * tan(M_PI / 4.0 + 0.5 * lat_rad);
        out[i].x = rho * sin(lon_rad);
        out[i].y = -rho * cos(lon_rad);
    }
}

void dc_geo_user_geodetic_to_polar_stereo(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3 *in, plVec2 *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        float lat_rad = in[i].x * (float)M_PI / 180.0f;
        float lon_rad = (180.0f - in[i].y) * (float)M_PI / 180.0f;
        float rho = 2.0f * (float)planet_radius * tanf((float)M_PI / 4.0f + 0.5f * lat_rad);
        out[i].x = rho * sinf(lon_rad);
        out[i].y = -rho * cosf(lon_rad);
    }
}

void dc_geo_user_geodetic_to_polar_stereo_d(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3d *in, plVec2d *out, size_t count) {
    (void)to;
    double planet_radius = from->planet_radius;
    for (size_t i = 0; i < count; i++) {
        double lat_rad = in[i].x * M_PI / 180.0;
        double lon_rad = (180.0 - in[i].y) * M_PI / 180.0;
        double rho = 2.0 * planet_radius * tan(M_PI / 4.0 + 0.5 * lat_rad);
        out[i].x = rho * sin(lon_rad);
        out[i].y = -rho * cos(lon_rad);
    }
}

// Attitude frames
void dc_geo_get_local_ned_basis(double lat_rad, double lon_rad, plVec3 *out_north, plVec3 *out_east, plVec3 *out_down, plVec3 *out_up) {
    double cos_lat = cos(lat_rad);
    double sin_lat = sin(lat_rad);
    double cos_lon = cos(lon_rad);
    double sin_lon = sin(lon_rad);

    *out_north = pl_norm_vec3((plVec3){
        (float)(-sin_lat * sin_lon),
        (float)cos_lat,
        (float)(-sin_lat * cos_lon)
    });

    *out_east = pl_norm_vec3((plVec3){
        (float)cos_lon,
        0.0f,
        (float)-sin_lon
    });

    *out_up = pl_norm_vec3((plVec3){
        (float)(cos_lat * sin_lon),
        (float)sin_lat,
        (float)(cos_lat * cos_lon)
    });

    *out_down = pl_mul_vec3_scalarf(*out_up, -1.0f);
}

// Vector operations
plVec3 dc_geo_rotate_vector_around_axis(plVec3 v, plVec3 axis, float angle) {
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    float one_minus_cos = 1.0f - cos_angle;

    // Rodrigues' rotation formula: v_rot = v*cos(θ) + (k×v)*sin(θ) + k(k·v)(1-cos(θ))
    // where k is the normalized axis
    plVec3 k = pl_norm_vec3(axis);
    float dot_kv = pl_dot_vec3(k, v);

    plVec3 cross = pl_cross_vec3(k, v);
    plVec3 part1 = pl_mul_vec3_scalarf(v, cos_angle);
    plVec3 part2 = pl_mul_vec3_scalarf(cross, sin_angle);
    plVec3 part3 = pl_mul_vec3_scalarf(k, dot_kv * one_minus_cos);

    return pl_add_vec3(pl_add_vec3(part1, part2), part3);
}

float dc_geo_signed_angle_around_axis(plVec3 from, plVec3 to, plVec3 axis) {
    plVec3 from_norm = pl_norm_vec3(from);
    plVec3 to_norm = pl_norm_vec3(to);
    plVec3 axis_norm = pl_norm_vec3(axis);

    float cos_angle = pl_dot_vec3(from_norm, to_norm);
    cos_angle = (cos_angle < -1.0f) ? -1.0f : (cos_angle > 1.0f) ? 1.0f : cos_angle;

    plVec3 cross = pl_cross_vec3(from_norm, to_norm);
    float sin_angle = pl_dot_vec3(cross, axis_norm);

    return atan2f(sin_angle, cos_angle);
}
