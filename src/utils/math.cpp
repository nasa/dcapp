// dcapp includes
#include <utils/math.hpp>

// library includes

// c++ standard includes
#include <cmath>

bool dc_utils_double_equals(double value1, double value2, double precision) {
    return fabs(value1 - value2) < precision;
}

bool dc_utils_float_equals(float value1, float value2, float precision) {
    return fabs(value1 - value2) < precision;
}

double dc_utils_degrees_to_radians(double degrees) {
    return degrees * 0.017453292519943295;
}

double dc_utils_radians_to_degrees(double radians) {
    return radians * 57.29577951308232;
}
