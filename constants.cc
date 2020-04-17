#include <list>
#include <string>
#include "types.hh"
#include "valuedata.hh"

static std::list<double> decimalConstants;
static std::list<int> integerConstants;
static std::list<std::string> stringConstants;

#if 0

double *dcLoadConstant(double fval)
{
    std::list<double>::iterator fc;
    for (fc = decimalConstants.begin(); fc != decimalConstants.end(); fc++)
    {
        if (*fc == fval) return &(*fc);
    }
    decimalConstants.push_back(fval);
    return &(decimalConstants.back());
}

int *dcLoadConstant(int ival)
{
    std::list<int>::iterator ic;
    for (ic = integerConstants.begin(); ic != integerConstants.end(); ic++)
    {
        if (*ic == ival) return &(*ic);
    }
    integerConstants.push_back(ival);
    return &(integerConstants.back());
}

#else

static std::list<ValueData> constants;

double *dcLoadConstant(double fval)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((double *)(it->getPointer())) == fval)
        {
//printf("DEC MATCH: %g\n", fval);
            return (double *)(it->getPointer());
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(DECIMAL_TYPE);
    vinfo->setValue(fval);
    constants.push_back(*vinfo);
//printf("DEC NEW: %g\n", fval);
    return (double *)(constants.back().getPointer());
}

int *dcLoadConstant(int ival)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((int *)(it->getPointer())) == ival)
        {
//printf("INT MATCH: %d\n", ival);
            return (int *)(it->getPointer());
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(INTEGER_TYPE);
    vinfo->setValue(ival);
    constants.push_back(*vinfo);
//printf("INT NEW: %d\n", ival);
    return (int *)(constants.back().getPointer());
}

ValueData *getConstantValue(int ival)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((int *)(it->getPointer())) == ival)
        {
//printf("INT MATCH: %d\n", ival);
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(INTEGER_TYPE);
    vinfo->setValue(ival);
    constants.push_back(*vinfo);
//printf("INT NEW: %d\n", ival);
    return &(constants.back());
}

#endif

std::string *dcLoadConstant(const char *sval)
{
    std::string myval;

    if (sval) myval = sval;
    else myval = "";

    std::list<std::string>::iterator sc;
    for (sc = stringConstants.begin(); sc != stringConstants.end(); sc++)
    {
        if (*sc == myval) return &(*sc);
    }
    stringConstants.push_back(myval);
    return &(stringConstants.back());
}
