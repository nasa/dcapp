#include "RenderLib/RenderLib.hh"
#include "valuedata.hh"
#include "string_utils.hh"
#include "panel.hh"

dcPanel::dcPanel(dcParent *myparent) : displayID(0)
{
    myparent->addChild(this);
    color.set(0, 0, 0);
    orthoX = getConstantValue(100);
    orthoY = getConstantValue(100);
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
    if (inw) orthoX = getValueData(inw);
    if (inh) orthoY = getValueData(inh);
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
}

ValueData * dcPanel::getContainerWidth(void) { return orthoX; }
ValueData * dcPanel::getContainerHeight(void) { return orthoY; }

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
