#ifndef DC_GEO_H
#define DC_GEO_H

#include <stddef.h>
#include "pl_math.h"

//~ coordinate systems

typedef struct DcGeoCrsGeodetic {
    double planet_radius;
} DcGeoCrsGeodetic;

typedef struct DcGeoCrsCartesian {
    double planet_radius;
} DcGeoCrsCartesian;

typedef struct DcGeoCrsPolarStereo {
    double planet_radius;
    double lat_origin;
    double lon_origin;
    double scale_factor;
    double false_easting;
    double false_northing;
} DcGeoCrsPolarStereo;

#ifdef __cplusplus
extern "C" {
#endif

//~ api

//- coordinate system creation
DcGeoCrsGeodetic dc_geo_create_crs_geodetic(double planet_radius);
DcGeoCrsCartesian dc_geo_create_crs_cartesian(double planet_radius);
DcGeoCrsPolarStereo dc_geo_create_crs_polar_stereographic(double planet_radius, double lat_origin, double lon_origin);

//- geodetic to cartesian

// input is latitude, longitude, and elevation in degrees and meters; output is cartesian xyz in meters
void dc_geo_geodetic_to_cartesian(const DcGeoCrsGeodetic *from, const DcGeoCrsCartesian *to, const plVec3 *in, plVec3 *out, size_t count);
void dc_geo_geodetic_to_cartesian_d(const DcGeoCrsGeodetic *from, const DcGeoCrsCartesian *to, const plVec3d *in, plVec3d *out, size_t count);

//- cartesian to geodetic

// input is cartesian xyz in meters; output is latitude, longitude, and elevation in degrees and meters
void dc_geo_cartesian_to_geodetic(const DcGeoCrsCartesian *from, const DcGeoCrsGeodetic *to, const plVec3 *in, plVec3 *out, size_t count);
void dc_geo_cartesian_to_geodetic_d(const DcGeoCrsCartesian *from, const DcGeoCrsGeodetic *to, const plVec3d *in, plVec3d *out, size_t count);

//- geodetic to polar stereographic

// input is latitude and longitude in degrees; output is projected xy in meters
void dc_geo_geodetic_to_polar_stereo(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3 *in, plVec2 *out, size_t count);
void dc_geo_geodetic_to_polar_stereo_d(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3d *in, plVec2d *out, size_t count);

//- legacy geodetic to polar stereographic

// convert degree latitude and mirrored user longitude to projected xy meters
void dc_geo_user_geodetic_to_polar_stereo(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3 *in, plVec2 *out, size_t count);
void dc_geo_user_geodetic_to_polar_stereo_d(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3d *in, plVec2d *out, size_t count);

//- attitude frames

// build normalized ecef north east down and up vectors from radian latitude and longitude
void dc_geo_get_local_ned_basis(double lat_rad, double lon_rad, plVec3 *out_north, plVec3 *out_east, plVec3 *out_down, plVec3 *out_up);

//- vector operations

// rotate a vector around an axis by an angle in radians
plVec3 dc_geo_rotate_vector_around_axis(plVec3 v, plVec3 axis, float angle);

// measure the signed angle around an axis in radians
float dc_geo_signed_angle_around_axis(plVec3 from, plVec3 to, plVec3 axis);

#ifdef __cplusplus
}
#endif

#endif
