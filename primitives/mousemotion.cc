#include "types.hh"
#include "varlist.hh"
#include "valuedata.hh"
#include "mousemotion.hh"

extern void UpdateDisplay(void);

dcMouseMotion::dcMouseMotion(dcParent *myparent, const char *xvar, const char *yvar)
{
    pointerX = &noval;
    ValueData *xval = getVariableValue(xvar);
    if (xval)
    {
        if (xval->isDecimal()) pointerX = xval;
    }

    pointerY = &noval;
    ValueData *yval = getVariableValue(yvar);
    if (yval)
    {
        if (yval->isDecimal()) pointerY = yval;
    }

    if (pointerX != &noval || pointerY != &noval) myparent->addChild(this);
}

dcMouseMotion::~dcMouseMotion() { }

void dcMouseMotion::handleMouseMotion(double inx, double iny)
{
    pointerX->setValue(inx);
    pointerY->setValue(iny);
    UpdateDisplay();
}
