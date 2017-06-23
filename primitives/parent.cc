#include <list>
#include "parent.hh"

dcParent::~dcParent()
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        delete *myobj;
    }
}

void dcParent::handleKeyboard(char key)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleKeyboard(key);
    }
}

void dcParent::handleMousePress(float x, float y)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleMousePress(x, y);
    }
}

void dcParent::handleMouseRelease(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleMouseRelease();
    }
}

void dcParent::handleBezelPress(int key)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleBezelPress(key);
    }
}

void dcParent::handleBezelRelease(int key)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleBezelRelease(key);
    }
}

void dcParent::updateData(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->updateData();
    }
}

void dcParent::processAnimation(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->processAnimation();
    }
}

void dcParent::addChild(dcObject *item)
{
    if (!item) return;
    this->children.push_back(item);
    item->setParent(this);
}
