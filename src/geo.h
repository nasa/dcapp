#ifndef _DC_UTILS_GEO_H_
#define _DC_UTILS_GEO_H_

#include <stdbool.h>
#include <stddef.h>
#include "pl_math.h"

// CRS definitions (complete coordinate system specifications)
typedef struct {
    double planet_radius;
} DcGeoCrsGeodetic;

typedef struct {
    double planet_radius;
} DcGeoCrsCartesian;

typedef struct {
    double planet_radius;
    double lat_origin;
    double lon_origin;
} DcGeoCrsPolarStereo;

#ifdef __cplusplus
extern "C" {
#endif

// Factory functions
DcGeoCrsGeodetic dc_geo_create_crs_geodetic(double planet_radius);
DcGeoCrsCartesian dc_geo_create_crs_cartesian(double planet_radius);
DcGeoCrsPolarStereo dc_geo_create_crs_polar_stereographic(double planet_radius, double lat_origin, double lon_origin);

// Coordinate conversions
// Geodetic: in->x=lat, in->y=lon, in->z=ele (degrees/meters)
// Cartesian: out->x/y/z (meters)
void dc_geo_geodetic_to_cartesian(const DcGeoCrsGeodetic *from, const DcGeoCrsCartesian *to, const plVec3 *in, plVec3 *out, size_t count);

// Cartesian: in->x/y/z (meters)
// Geodetic: out->x=lat, out->y=lon, out->z=ele (degrees/meters)
void dc_geo_cartesian_to_geodetic(const DcGeoCrsCartesian *from, const DcGeoCrsGeodetic *to, const plVec3 *in, plVec3 *out, size_t count);

// Geodetic: in->x=lat, in->y=lon (degrees)
// PolarStereo: out->x/y=projected (meters)
void dc_geo_geodetic_to_polar_stereo(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3 *in, plVec2 *out, size_t count);

// Geodetic with longitude convention flip: in->x=lat, in->y=lon (degrees)
// PolarStereo: out->x/y=projected (meters)
void dc_geo_user_geodetic_to_polar_stereo(const DcGeoCrsGeodetic *from, const DcGeoCrsPolarStereo *to, const plVec3 *in, plVec2 *out, size_t count);

// Attitude frames
// Get local NED (North-East-Down) basis vectors for a geodetic position.
// lat_rad, lon_rad: geodetic latitude/longitude in radians
// Returns: normalized basis vectors in ECEF coordinates (north, east, down, up)
void dc_geo_get_local_ned_basis(double lat_rad, double lon_rad, plVec3 *out_north, plVec3 *out_east, plVec3 *out_down, plVec3 *out_up);

// Vector operations
// Rotate vector v around axis by angle (in radians)
plVec3 dc_geo_rotate_vector_around_axis(plVec3 v, plVec3 axis, float angle);

// Get signed angle from 'from' to 'to' around 'axis' (in radians)
float dc_geo_signed_angle_around_axis(plVec3 from, plVec3 to, plVec3 axis);

#ifdef __cplusplus
}
#endif

#endif
