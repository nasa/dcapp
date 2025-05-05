// dcapp includes
#include <utils/math-utils.hpp>

// library includes

// c++ standard includes
#include <cmath>

bool doubleEquals(double value1, double value2, double precision)
{
    return fabs(value1 - value2) < precision;
}

bool floatEquals(float value1, float value2, float precision)
{
    return fabs(value1 - value2) < precision;
}
