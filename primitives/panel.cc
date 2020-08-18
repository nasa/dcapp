#include <string>
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

void dcPanel::setID(const std::string &inval)
{
    if (!inval.empty()) displayID = StringToInteger(inval);
}

void dcPanel::setColor(const std::string &cspec)
{
    if (!cspec.empty()) color.set(cspec);
}

void dcPanel::setOrtho(const std::string &inw, const std::string &inh)
{
    if (!inw.empty()) orthoX = getValueSSTR(inw);
    if (!inh.empty()) orthoY = getValueSSTR(inh);
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
