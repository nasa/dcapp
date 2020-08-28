#include <string>
#include "variables.hh"
#include "mousemotion.hh"

extern void UpdateDisplay(void);

dcMouseMotion::dcMouseMotion(dcParent *myparent, const std::string &xvar, const std::string &yvar)
{
    pointerX = &noval;
    Variable *xval = getVariableSSTR(xvar);
    if (xval)
    {
        if (xval->isDecimal()) pointerX = xval;
    }

    pointerY = &noval;
    Variable *yval = getVariableSSTR(yvar);
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
