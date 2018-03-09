#include "types.hh"
#include "varlist.hh"
#include "mousemotion.hh"

extern void UpdateDisplay(void);

dcMouseMotion::dcMouseMotion(dcParent *myparent, const char *xvar, const char *yvar)
:
pointerX(nullptr), pointerY(nullptr)
{
    int datatype;

    datatype = get_data_type(xvar);
    if (datatype == DECIMAL_TYPE)
        pointerX = (double *)getVariablePointer(datatype, xvar);
    else
        pointerX = &noval;

    datatype = get_data_type(yvar);
    if (datatype == DECIMAL_TYPE)
        pointerY = (double *)getVariablePointer(datatype, yvar);
    else
        pointerY = &noval;

    if (pointerX != &noval || pointerY != &noval) myparent->addChild(this);
}

dcMouseMotion::~dcMouseMotion()
{
}

void dcMouseMotion::handleMouseMotion(double inx, double iny)
{
    *pointerX = inx;
    *pointerY = iny;
    UpdateDisplay();
}
