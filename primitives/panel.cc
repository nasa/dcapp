#include "opengl_draw.hh"
#include "string_utils.hh"
#include "panel.hh"

dcPanel::dcPanel(dcParent *myparent) : displayID(0), orthoX(100), orthoY(100)
{
    myparent->addChild(this);
    color.set(0, 0, 0);
}

void dcPanel::setID(const char *inval)
{
    if (inval) displayID = StrToInt(inval, 0);
}

void dcPanel::setColor(const char *cspec)
{
    color.set(cspec);
}

void dcPanel::setOrtho(const char *inw, const char *inh)
{
    if (inw) orthoX = StrToFloat(inw, 100);
    if (inh) orthoY = StrToFloat(inh, 100);
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
}

float * dcPanel::getContainerWidth(void)
{
    return &orthoX;
}

float * dcPanel::getContainerHeight(void)
{
    return &orthoY;
}

void dcPanel::draw(void)
{
    setup_panel(orthoX, orthoY, -1, 1, *(color.R), *(color.G), *(color.B), *(color.A));
    for (const auto &myobj : children) myobj->draw();
}

void dcPanel::handleMousePress(float x, float y)
{
    for (const auto &myobj : children) myobj->handleMousePress(orthoX * x, orthoY * y);
}
