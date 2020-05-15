#include <list>
#include "valuedata.hh"

static std::list<Constant> constants;

static Constant *findOrCreateConstant(Constant &vinfo)
{
    std::list<Constant>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*it == vinfo) return &(*it);
    }
    constants.push_back(vinfo);
    return &(constants.back());
}

Constant *getConstantFromDecimal(double inval)
{
    Constant vinfo;
    vinfo.setToDecimal(inval);
    return findOrCreateConstant(vinfo);
}

Constant *getConstantFromInteger(int inval)
{
    Constant vinfo;
    vinfo.setToInteger(inval);
    return findOrCreateConstant(vinfo);
}

Constant *getConstantFromCharstr(const char *inval)
{
    Constant vinfo;
    vinfo.setToCharstr(inval);
    return findOrCreateConstant(vinfo);
}

Constant *getConstantFromBoolean(bool inval)
{
    Constant vinfo;
    vinfo.setToBoolean(inval);
    return findOrCreateConstant(vinfo);
}
