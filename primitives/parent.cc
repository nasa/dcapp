#include <list>
#include "parent.hh"

dcParent::~dcParent()
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        delete *myobj;
    }
}

void dcParent::completeInitialization(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->completeInitialization();
    }
}

void dcParent::draw(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->draw();
    }
}

void dcParent::handleKeyPress(char key)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleKeyPress(key);
    }
}

void dcParent::handleKeyRelease(char key)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleKeyRelease(key);
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

void dcParent::handleEvent(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleEvent();
    }
}

void dcParent::updateData(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->updateData();
    }
}

void dcParent::updateStreams(unsigned passcount)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->updateStreams(passcount);
    }
}

void dcParent::processAnimation(Animation *anim)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->processAnimation(anim);
    }
}

void dcParent::addChild(dcObject *item)
{
    if (!item) return;
    this->children.push_back(item);
    item->setParent(this);
}
