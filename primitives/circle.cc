#include <string>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "constants.hh"
#include "values.hh"
#include "circle.hh"

extern void RegisterPressedPrimitive(dcParent *);

dcCircle::dcCircle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false), segments(80), selected(false), linePattern(0xFFFF), lineFactor(1)
{
    radius = getConstantFromDecimal(0);
    FillColor.set(0.5, 0.5, 0.5);
    LineColor.set(1, 1, 1);

    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

dcCircle::~dcCircle()
{
    delete PressList;
    delete ReleaseList;
}

void dcCircle::setFillColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        FillColor.set(cspec);
        fill = true;
    }
}

void dcCircle::setLineColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        LineColor.set(cspec);
        outline = true;
    }
}

void dcCircle::setLineWidth(const std::string &inval)
{
    if (!inval.empty())
    {
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcCircle::setLinePattern(const std::string &inval)
{
    if (!inval.empty()) 
    {
        linePattern = HexStringToInteger(inval, 0xFFFF);
    }
}

void dcCircle::setLineFactor(const std::string &inval)
{
    if (!inval.empty())
    { 
        lineFactor = StringToInteger(inval, 1);
    }
}

void dcCircle::setRadius(const std::string &inval)
{
    if (!inval.empty()) radius = getValue(inval);
}

void dcCircle::setSegments(const std::string &inval)
{
    if (!inval.empty()) segments = StringToInteger(inval, 80);
}

void dcCircle::handleMousePress(double inx, double iny)
{
    if (this->PressList->children.empty() && this->ReleaseList->children.empty()) return;

    computeGeometry();

    double deltax = inx - refx;
    double deltay = iny - refy;

    if ((deltax * deltax) + (deltay * deltay) <= (radius->getDecimal() * radius->getDecimal()))
    {
        this->selected = true;
        this->PressList->handleEvent();
        RegisterPressedPrimitive(this->PressList);
    }
}

void dcCircle::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
    }
}

void dcCircle::processPreCalculations(void) 
{
    PressList->processPreCalculations();
    ReleaseList->processPreCalculations();
}

void dcCircle::processPostCalculations(void) 
{
    PressList->processPostCalculations();
    ReleaseList->processPostCalculations();
}

void dcCircle::draw(void)
{
    computeGeometry();
    if (fill) circle_fill(refx, refy, radius->getDecimal(), segments, FillColor.R->getDecimal(), FillColor.G->getDecimal(), FillColor.B->getDecimal(), FillColor.A->getDecimal());
    if (outline) circle_outline(refx, refy, radius->getDecimal(), segments, LineColor.R->getDecimal(), LineColor.G->getDecimal(), LineColor.B->getDecimal(), LineColor.A->getDecimal(), linewidth, linePattern, lineFactor);
}
