#include "setvalue.hh"

// TODO: put in a centralized header file:
extern void UpdateValueLogic(int, int, void *, int, void *, int, void *, int, void *);

dcSetValue::dcSetValue(int opspec, int dtype1, int dtype2, int mindtype, int maxdtype, void *varspec, void *valspec, void *minspec, void *maxspec)
{
    optype = opspec;
    datatype1 = dtype1;
    datatype2 = dtype2;
    mindatatype = mindtype;
    maxdatatype = maxdtype;
    var = varspec;
    val = valspec;
    min = minspec;
    max = maxspec;
}

void dcSetValue::draw(void)
{
    UpdateValueLogic(optype, datatype1, var, datatype2, val, mindatatype, min, maxdatatype, max);
}

void dcSetValue::updateData(void)
{
    UpdateValueLogic(optype, datatype1, var, datatype2, val, mindatatype, min, maxdatatype, max);
}
