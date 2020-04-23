#include <list>
#include <string>
#include "types.hh"
#include "valuedata.hh"

static std::list<ValueData> constants;

#if 0

ValueData *getConstantValue(double fval)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getDecimal() == fval)
        {
//printf("cvDEC MATCH: %g\n", fval);
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(DECIMAL_TYPE);
    vinfo->setValue(fval);
    vinfo->makeGeneric();
    constants.push_back(*vinfo);
//printf("cvDEC NEW: %g\n", fval);
    return &(constants.back());
}

ValueData *getConstantValue(int ival)
{
    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getInteger() == ival)
        {
//printf("cvINT MATCH: %d\n", ival);
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(INTEGER_TYPE);
    vinfo->setValue(ival);
    vinfo->makeGeneric();
    constants.push_back(*vinfo);
//printf("cvINT NEW: %d\n", ival);
    return &(constants.back());
}

ValueData *getConstantValue(const char *sval)
{
    std::string myval;
    if (sval) myval = sval;

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getString() == myval)
        {
//printf("cvSTR MATCH: %s\n", myval.c_str());
            return &(*it);
        }
    }
    ValueData *vinfo = new ValueData;
    vinfo->setType(STRING_TYPE);
    vinfo->setValue(myval);
    vinfo->makeGeneric();
    constants.push_back(*vinfo);
//printf("cvSTR NEW: %s\n", myval.c_str());
    return &(constants.back());
}

#else

ValueData *getConstantValue(double fval)
{
    ValueData *vinfo = new ValueData;
    vinfo->setType(DECIMAL_TYPE);
    vinfo->setValue(fval);
    vinfo->makeGeneric();

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getDecimal() == vinfo->getDecimal() && it->getInteger() == vinfo->getInteger() && it->getString() == vinfo->getString())
        {
            delete vinfo;
            return &(*it);
        }
    }
    constants.push_back(*vinfo);
    return &(constants.back());
}

ValueData *getConstantValue(int ival)
{
    ValueData *vinfo = new ValueData;
    vinfo->setType(INTEGER_TYPE);
    vinfo->setValue(ival);
    vinfo->makeGeneric();

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getDecimal() == vinfo->getDecimal() && it->getInteger() == vinfo->getInteger() && it->getString() == vinfo->getString())
        {
            delete vinfo;
            return &(*it);
        }
    }
    constants.push_back(*vinfo);
    return &(constants.back());
}

ValueData *getConstantValue(const char *sval)
{
    if (!sval) return 0x0;
    std::string myval = sval;

    ValueData *vinfo = new ValueData;
    vinfo->setType(STRING_TYPE);
    vinfo->setValue(myval);
    vinfo->makeGeneric();

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getDecimal() == vinfo->getDecimal() && it->getInteger() == vinfo->getInteger() && it->getString() == vinfo->getString())
        {
            delete vinfo;
            return &(*it);
        }
    }
    constants.push_back(*vinfo);
    return &(constants.back());
}

#endif
