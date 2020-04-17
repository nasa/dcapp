#include "RenderLib/RenderLib.hh"
#include "string_utils.hh"
#include "panel.hh"

dcPanel::dcPanel(dcParent *myparent) : displayID(0), orthoX(100), orthoY(100)
{
    myparent->addChild(this);
    color.set(0, 0, 0);
}

void dcPanel::setID(const char *inval)
{
    if (inval) displayID = StringToInteger(inval);
}

void dcPanel::setColor(const char *cspec)
{
    color.set(cspec);
}

void dcPanel::setOrtho(const char *inw, const char *inh)
{
    if (inw) orthoX = StringToDecimal(inw, 100);
    if (inh) orthoY = StringToDecimal(inh, 100);
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
}

double * dcPanel::getContainerWidth(void)
{
    return &orthoX;
}

double * dcPanel::getContainerHeight(void)
{
    return &orthoY;
}

void dcPanel::draw(void)
{
    setup_panel(orthoX, orthoY, color.R->getDecimal(), color.G->getDecimal(), color.B->getDecimal(), color.A->getDecimal());
    for (const auto &myobj : children) myobj->draw();
}

void dcPanel::handleMousePress(double x, double y)
{
    for (const auto &myobj : children) myobj->handleMousePress(orthoX * x, orthoY * y);
}

void dcPanel::handleMouseMotion(double x, double y)
{
    for (const auto &myobj : children) myobj->handleMouseMotion(orthoX * x, orthoY * y);
}
