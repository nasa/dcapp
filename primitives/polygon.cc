#include <string>
#include "app_data.hh"
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "polygon.hh"

extern void RegisterPressedPrimitive(dcParent *);

extern appdata AppData;

dcPolygon::dcPolygon(dcParent *myparent) : linewidth(1), fill(false), outline(false), selected(false), linePattern(0xFFFF), lineFactor(1)
{
    myparent->addChild(this);
    FillColor.set(0.5, 0.5, 0.5);
    LineColor.set(1, 1, 1);

    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

dcPolygon::~dcPolygon()
{
    delete PressList;
    delete ReleaseList;
}

void dcPolygon::setFillColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        FillColor.set(cspec);
        fill = true;
    }
}

void dcPolygon::setLineColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        LineColor.set(cspec);
        outline = true;
    }
}

void dcPolygon::setLineWidth(const std::string &inval)
{
    if (!inval.empty())
    {
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcPolygon::setLinePattern(const std::string &inval)
{
    if (!inval.empty()) 
    {
        linePattern = HexStringToInteger(inval, 0xFFFF);
    }
}

void dcPolygon::setLineFactor(const std::string &inval)
{
    if (!inval.empty())
    { 
        lineFactor = StringToInteger(inval, 1);
    }
}

void dcPolygon::handleMousePress(double inx, double iny)
{
    if (this->PressList->children.empty() && this->ReleaseList->children.empty()) return;

    unsigned i, j, nvert = this->vertices.size()/2;
    bool inpoly = false;

// these simply improve readability of the ray tracing logic below...
#define VX(a) this->vertices[(2*a)]
#define VY(a) this->vertices[(2*a)+1]

    // this is ray-tracing logic, which looks complex but is very clean and efficient
    for (i=0, j=nvert-1; i<nvert; j=i++)
    {
        if (((VY(i) >= iny) != (VY(j) >= iny)) && (inx <= ((VX(j) - VX(i)) * (iny - VY(i)) / (VY(j) - VY(i))) + VX(i)))
            inpoly = !inpoly;
    }

    if (inpoly)
    {
        this->selected = true;
        this->PressList->handleEvent();
        RegisterPressedPrimitive(this->PressList);
    }
}

void dcPolygon::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
    }
}

void dcPolygon::draw(void)
{
    // this effectively fills AppData.vertices with the individual Vertex primitives associated with this Polygon 
    for (const auto &myobj : children) 
    {
        myobj->processPreCalculations();
        myobj->draw();
        myobj->processPostCalculations();
    }

    // this copies the vertices from AppData to this for subsequent usage, like handleMousePress
    this->vertices = AppData.vertices;

    if (fill)
        draw_polygon(this->vertices, FillColor.R->getDecimal(), FillColor.G->getDecimal(), FillColor.B->getDecimal(), FillColor.A->getDecimal());
    if (outline)
        draw_line(this->vertices, linewidth, LineColor.R->getDecimal(), LineColor.G->getDecimal(), LineColor.B->getDecimal(), LineColor.A->getDecimal(), linePattern, lineFactor);

    // this clears AppData.vertices for the next primitive (Polygon or Line) that uses it
    AppData.vertices.clear();
}
