#include "nodes.hh"
#include "varlist.hh"
#include "vertex.hh"

extern appdata AppData;

dcVertex::dcVertex(dcParent *myparent) : x(0x0), y(0x0), originx(dcLeft), originy(dcBottom)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
}

void dcVertex::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getValue(inx);
    if (iny) y = getValue(iny);
}

void dcVertex::setOrigin(const char *inx, const char *iny)
{
    if (inx)
    {
        if (!strcasecmp(inx, "Left")) originx = dcLeft;
        else if (!strcasecmp(inx, "Center")) originx = dcCenter;
        else if (!strcasecmp(inx, "Right")) originx = dcRight;
    }
    if (iny)
    {
        if (!strcasecmp(iny, "Bottom")) originy = dcBottom;
        else if (!strcasecmp(iny, "Middle")) originy = dcMiddle;
        else if (!strcasecmp(iny, "Top")) originy = dcTop;
    }
}

void dcVertex::draw(void)
{
    double myx, myy;

    if (x)
    {
//if (x->getDecimal() < 0) printf("WARNING: X in Vertex (%g)\n", x->getDecimal());
if (originx == dcRight) myx = containerw->getDecimal() - x->getDecimal();
else myx = x->getDecimal();
    }
    else myx = 0;

    if (y)
    {
//if (y->getDecimal() < 0) printf("WARNING: Y in Vertex (%g)\n", y->getDecimal());
if (originy == dcTop) myy = containerh->getDecimal() - y->getDecimal();
else myy = y->getDecimal();
    }
    else myy = 0;

    AppData.vertices.push_back(myx);
    AppData.vertices.push_back(myy);
}
