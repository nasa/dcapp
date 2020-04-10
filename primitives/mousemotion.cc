#include "types.hh"
#include "varlist.hh"
#include "valuedata.hh"
#include "mousemotion.hh"

extern void UpdateDisplay(void);

dcMouseMotion::dcMouseMotion(dcParent *myparent, const char *xvar, const char *yvar)
:
noval(0.0)
{
    pointerX = &noval;
    ValueData *xval = getValue(xvar);
    if (xval)
    {
        if (xval->isDecimal()) pointerX = (double *)(xval->getPointer());
    }

    pointerY = &noval;
    ValueData *yval = getValue(yvar);
    if (yval)
    {
        if (yval->isDecimal()) pointerY = (double *)(yval->getPointer());
    }

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
