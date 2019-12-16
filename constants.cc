#include <list>
#include <string>

static std::list<double> decimalConstants;
static std::list<int> integerConstants;
static std::list<std::string> stringConstants;

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
