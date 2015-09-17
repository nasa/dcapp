#include <string.h>
#include <list>

static std::list<float> floatConstants;
static std::list<int> integerConstants;
static std::list<char *> stringConstants;

float *LoadConstant(float fval)
{
    std::list<float>::iterator fc;
    for (fc = floatConstants.begin(); fc != floatConstants.end(); fc++)
    {
        if (*fc == fval) return &(*fc);
    }
    floatConstants.push_back(fval);
    return &(floatConstants.back());
}

int *LoadConstant(int ival)
{
    std::list<int>::iterator ic;
    for (ic = integerConstants.begin(); ic != integerConstants.end(); ic++)
    {
        if (*ic == ival) return &(*ic);
    }
    integerConstants.push_back(ival);
    return &(integerConstants.back());
}

char *LoadConstant(const char *sval)
{
    std::list<char *>::iterator sc;
    for (sc = stringConstants.begin(); sc != stringConstants.end(); sc++)
    {
        if (!strcmp(*sc, sval)) return *sc;
    }
    stringConstants.push_back(strdup(sval));
    return stringConstants.back();
}
