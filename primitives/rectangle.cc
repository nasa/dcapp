#include <string>
#include <vector>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "rectangle.hh"

extern void RegisterPressedPrimitive(dcParent *);

dcRectangle::dcRectangle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false), selected(false)
{
    FillColor.set(0.5, 0.5, 0.5);
    LineColor.set(1, 1, 1);

    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

dcRectangle::~dcRectangle()
{
    delete PressList;
    delete ReleaseList;
}

void dcRectangle::setFillColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        FillColor.set(cspec);
        fill = true;
    }
}

void dcRectangle::setLineColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        LineColor.set(cspec);
        outline = true;
    }
}

void dcRectangle::setLineWidth(const std::string &inval)
{
    if (!inval.empty())
    {
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcRectangle::handleMousePress(double inx, double iny)
{
    if (this->PressList->children.empty() && this->ReleaseList->children.empty()) return;

    double finalx, finaly;

    computeGeometry();
    if (rotate->getDecimal())
    {
        double ang = (rotate->getDecimal()) * 0.01745329252;
        double originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
        double originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
        double tmpx = inx - originx;
        double tmpy = iny - originy;
        finalx = (tmpx * cos(ang)) + (tmpy * sin(ang));
        finaly = (tmpy * cos(ang)) - (tmpx * sin(ang));
    }
    else
    {
        finalx = inx + delx - refx;
        finaly = iny + dely - refy;
    }

    if ((0 < finalx) && (finalx < width) && (0 < finaly) && (finaly < height))
    {
        this->selected = true;
        this->PressList->handleEvent();
        RegisterPressedPrimitive(this->PressList);
    }
}

void dcRectangle::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
    }
}

void dcRectangle::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, rotate->getDecimal());
    std::vector<float> pointsL;
    pointsL.reserve(8);

    addPoint(pointsL, 0, 0);
    addPoint(pointsL, 0, height);
    addPoint(pointsL, width, height);
    addPoint(pointsL, width, 0);

    if (fill)
    {
        draw_quad(pointsL, FillColor.R->getDecimal(), FillColor.G->getDecimal(), FillColor.B->getDecimal(), FillColor.A->getDecimal());
    }
    if (outline)
    {
        addPoint(pointsL, 0, 0);
        draw_line(pointsL, linewidth, LineColor.R->getDecimal(), LineColor.G->getDecimal(), LineColor.B->getDecimal(), LineColor.A->getDecimal());
    }
    container_end();
}
