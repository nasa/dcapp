#include <list>
#include "parent.hh"

dcParent::~dcParent()
{
    for (const auto &myobj : children) delete myobj;
}

void dcParent::draw(void)
{
    for (const auto &myobj : children) myobj->draw();
}

void dcParent::handleKeyPress(char key)
{
    for (const auto &myobj : children) myobj->handleKeyPress(key);
}

void dcParent::handleKeyRelease(char key)
{
    for (const auto &myobj : children) myobj->handleKeyRelease(key);
}

void dcParent::handleMousePress(double x, double y)
{
    for (const auto &myobj : children) myobj->handleMousePress(x, y);
}

void dcParent::handleMouseRelease(void)
{
    for (const auto &myobj : children) myobj->handleMouseRelease();
}

void dcParent::handleBezelPress(int key)
{
    for (const auto &myobj : children) myobj->handleBezelPress(key);
}

void dcParent::handleBezelRelease(int key)
{
    for (const auto &myobj : children) myobj->handleBezelRelease(key);
}

void dcParent::handleEvent(void)
{
    for (const auto &myobj : children) myobj->handleEvent();
}

void dcParent::updateData(void)
{
    for (const auto &myobj : children) myobj->updateData();
}

void dcParent::updateStreams(unsigned passcount)
{
    for (const auto &myobj : children) myobj->updateStreams(passcount);
}

void dcParent::processAnimation(Animation *anim)
{
    for (const auto &myobj : children) myobj->processAnimation(anim);
}

void dcParent::addChild(dcObject *item)
{
    if (!item) return;
    children.push_back(item);
    item->setParent(this);
}
