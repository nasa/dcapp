#include "math.h"

#include <math.h>

#define DC_DEG_TO_RAD 0.017453292519943295 // M_PI / 180.0
#define DC_RAD_TO_DEG 57.29577951308232    // 180.0 / M_PI

bool dc_utils_double_equals(double value1, double value2, double precision) {
    return fabs(value1 - value2) < precision;
}

bool dc_utils_float_equals(float value1, float value2, float precision) {
    return fabs(value1 - value2) < precision;
}

double dc_utils_degrees_to_radians(double degrees) {
    return degrees * DC_DEG_TO_RAD;
}

double dc_utils_radians_to_degrees(double radians) {
    return radians * DC_RAD_TO_DEG;
}
