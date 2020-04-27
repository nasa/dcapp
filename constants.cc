#include <list>
#include <string>
#include "types.hh"
#include "valuedata.hh"

static std::list<ValueData> constants;

static ValueData *getConstantValueData(ValueData &vinfo)
{
    vinfo.makeGeneric();

    std::list<ValueData>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (it->getDecimal() == vinfo.getDecimal() && it->getInteger() == vinfo.getInteger() && it->getString() == vinfo.getString())
        {
            return &(*it);
        }
    }
    constants.push_back(vinfo);
    return &(constants.back());
}

ValueData *getConstantValue(double fval)
{
    ValueData vinfo;
    vinfo.setType(DECIMAL_TYPE);
    vinfo.setValue(fval);
    return getConstantValueData(vinfo);
}

ValueData *getConstantValue(int ival)
{
    ValueData vinfo;
    vinfo.setType(INTEGER_TYPE);
    vinfo.setValue(ival);
    return getConstantValueData(vinfo);
}

ValueData *getConstantValue(const char *sval)
{
    std::string myval;
    if (sval) myval = sval;

    ValueData vinfo;
    vinfo.setType(STRING_TYPE);
    vinfo.setValue(sval);
    return getConstantValueData(vinfo);
}
