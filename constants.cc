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

Constant *getConstant(double inval)
{
    Constant vinfo;
    vinfo.setValue(inval);
    return findOrCreateConstant(vinfo);
}

Constant *getConstant(int inval)
{
    Constant vinfo;
    vinfo.setValue(inval);
    return findOrCreateConstant(vinfo);
}

Constant *getConstant(const char *inval)
{
    Constant vinfo;
    vinfo.setValue(inval);
    return findOrCreateConstant(vinfo);
}

Constant *getConstant(bool inval)
{
    Constant vinfo;
    vinfo.setValue(inval);
    return findOrCreateConstant(vinfo);
}
