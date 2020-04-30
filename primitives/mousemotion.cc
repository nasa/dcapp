#include "varlist.hh"
#include "valuedata.hh"
#include "mousemotion.hh"

extern void UpdateDisplay(void);

dcMouseMotion::dcMouseMotion(dcParent *myparent, const char *xvar, const char *yvar)
{
    pointerX = &noval;
    Variable *xval = getVariable(xvar);
    if (xval)
    {
        if (xval->isDecimal()) pointerX = xval;
    }

    pointerY = &noval;
    Variable *yval = getVariable(yvar);
    if (yval)
    {
        if (yval->isDecimal()) pointerY = yval;
    }

    if (pointerX != &noval || pointerY != &noval) myparent->addChild(this);
}

dcMouseMotion::~dcMouseMotion() { }

void dcMouseMotion::handleMouseMotion(double inx, double iny)
{
    pointerX->setToDecimal(inx);
    pointerY->setToDecimal(iny);
    UpdateDisplay();
}
