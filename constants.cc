#include <list>
#include <string>
#include "types.hh"
#include "valuedata.hh"

static std::list<ValueData> constants;

double *dcLoadConstant(double fval)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((double *)(it->getPointer())) == fval)
        {
printf("DEC MATCH: %g\n", fval);
            return (double *)(it->getPointer());
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(DECIMAL_TYPE);
    vinfo->setValue(fval);
    constants.push_back(*vinfo);
printf("DEC NEW: %g\n", fval);
    return (double *)(constants.back().getPointer());
}

int *dcLoadConstant(int ival)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((int *)(it->getPointer())) == ival)
        {
printf("INT MATCH: %d\n", ival);
            return (int *)(it->getPointer());
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(INTEGER_TYPE);
    vinfo->setValue(ival);
    constants.push_back(*vinfo);
printf("INT NEW: %d\n", ival);
    return (int *)(constants.back().getPointer());
}

std::string *dcLoadConstant(const char *sval)
{
    std::string myval;
    if (sval) myval = sval;

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((std::string *)(it->getPointer())) == myval)
        {
printf("STR MATCH: %s\n", myval.c_str()); fflush(0);
            return (std::string *)(it->getPointer());
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(STRING_TYPE);
    vinfo->setValue(myval);
    constants.push_back(*vinfo);
printf("STR NEW: %s\n", myval.c_str()); fflush(0);
    return (std::string *)(constants.back().getPointer());
}

ValueData *getConstantValue(double fval)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((double *)(it->getPointer())) == fval)
        {
printf("cvDEC MATCH: %g\n", fval);
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(DECIMAL_TYPE);
    vinfo->setValue(fval);
    constants.push_back(*vinfo);
printf("cvDEC NEW: %g\n", fval);
    return &(constants.back());
}

ValueData *getConstantValue(int ival)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((int *)(it->getPointer())) == ival)
        {
printf("cvINT MATCH: %d\n", ival);
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(INTEGER_TYPE);
    vinfo->setValue(ival);
    constants.push_back(*vinfo);
printf("cvINT NEW: %d\n", ival);
    return &(constants.back());
}

ValueData *getConstantValue(const char *sval)
{
    std::string myval;
    if (sval) myval = sval;

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*((std::string *)(it->getPointer())) == myval)
        {
printf("cvSTR MATCH: %s\n", myval.c_str());
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(STRING_TYPE);
    vinfo->setValue(myval);
    constants.push_back(*vinfo);
printf("cvSTR NEW: %s\n", myval.c_str());
    return &(constants.back());
}
