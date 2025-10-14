#ifndef _DC_UTILS_MATH_
#define _DC_UTILS_MATH_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool   dc_utils_double_equals(double value1, double value2, double precision);
bool   dc_utils_float_equals(float value1, float value2, float precision);
double dc_utils_degrees_to_radians(double degrees);
double dc_utils_radians_to_degrees(double radians);

#ifdef __cplusplus
}
#endif

#endif
