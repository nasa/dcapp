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
