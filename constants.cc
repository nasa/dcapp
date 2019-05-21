#include <list>
#include <cstring>

static std::list<double> decimalConstants;
static std::list<int> integerConstants;
static std::list<char *> stringConstants;

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

char *dcLoadConstant(const char *sval)
{
    const char *myval;
    const char nulval='\0';

    if (sval) myval = sval;
    else myval = &nulval;

    std::list<char *>::iterator sc;
    for (sc = stringConstants.begin(); sc != stringConstants.end(); sc++)
    {
        if (!strcmp(*sc, myval)) return *sc;
    }
    stringConstants.push_back(strdup(myval));
    return stringConstants.back();
}
