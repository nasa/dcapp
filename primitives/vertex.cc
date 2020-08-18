#include <string>
#include "basicutils/stringutils.hh"
#include "app_data.hh"
#include "values.hh"
#include "vertex.hh"

extern appdata AppData;

dcVertex::dcVertex(dcParent *myparent) : x(0x0), y(0x0), originx(dcLeft), originy(dcBottom)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
}

void dcVertex::setPosition(const std::string &inx, const std::string &iny)
{
    if (!inx.empty()) x = getValueSSTR(inx);
    if (!iny.empty()) y = getValueSSTR(iny);
}

void dcVertex::setOrigin(const std::string &inx, const std::string &iny)
{
    if (!inx.empty())
    {
        if (CaseInsensitiveCompare(inx, "Left")) originx = dcLeft;
        else if (CaseInsensitiveCompare(inx, "Center")) originx = dcCenter;
        else if (CaseInsensitiveCompare(inx, "Right")) originx = dcRight;
    }
    if (!iny.empty())
    {
        if (CaseInsensitiveCompare(iny, "Bottom")) originy = dcBottom;
        else if (CaseInsensitiveCompare(iny, "Middle")) originy = dcMiddle;
        else if (CaseInsensitiveCompare(iny, "Top")) originy = dcTop;
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
