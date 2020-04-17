#include "nodes.hh"
#include "varlist.hh"
#include "vertex.hh"

extern appdata AppData;

dcVertex::dcVertex(dcParent *myparent) : x(0x0), y(0x0)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
}

void dcVertex::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getValueData(inx);
    if (iny) y = getValueData(iny);
}

void dcVertex::draw(void)
{
    double myx, myy;

    if (x)
    {
        if (x->getDecimal() < 0) myx = x->getDecimal() + *containerw;
        else myx = x->getDecimal();
    }
    else myx = 0;

    if (y)
    {
        if (y->getDecimal() < 0) myy = y->getDecimal() + *containerh;
        else myy = y->getDecimal();
    }
    else myy = 0;

    AppData.vertices.push_back(myx);
    AppData.vertices.push_back(myy);
}
