#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "constants.hh"
#include "values.hh"
#include "panel.hh"

dcPanel::dcPanel(dcParent *myparent) : displayID(0)
{
    myparent->addChild(this);
    color.set(0, 0, 0);
    orthoX = getConstantFromDecimal(100);
    orthoY = getConstantFromDecimal(100);
}

void dcPanel::setID(const char *inval)
{
    if (inval) displayID = StringToInteger(inval);
}

void dcPanel::setColor(const char *cspec)
{
    if (cspec) color.set(cspec);
}

void dcPanel::setOrtho(const char *inw, const char *inh)
{
    if (inw) orthoX = getValue(inw);
    if (inh) orthoY = getValue(inh);
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
}

Value * dcPanel::getContainerWidth(void) { return orthoX; }
Value * dcPanel::getContainerHeight(void) { return orthoY; }

void dcPanel::draw(void)
{
    setup_panel(orthoX->getDecimal(), orthoY->getDecimal(), color.R->getDecimal(), color.G->getDecimal(), color.B->getDecimal(), color.A->getDecimal());
    for (const auto &myobj : children) myobj->draw();
}

void dcPanel::handleMousePress(double x, double y)
{
    for (const auto &myobj : children) myobj->handleMousePress(orthoX->getDecimal() * x, orthoY->getDecimal() * y);
}

void dcPanel::handleMouseMotion(double x, double y)
{
    for (const auto &myobj : children) myobj->handleMouseMotion(orthoX->getDecimal() * x, orthoY->getDecimal() * y);
}
