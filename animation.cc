#include <list>
#include "animation.hh"

Animation::Animation()
{
}

Animation::~Animation()
{
    std::list<AnimationItem *>::iterator myitem;

    for (myitem = this->items.begin(); myitem != this->items.end(); myitem++)
    {
        delete (*myitem);
    }
}

void Animation::initialize(double start, double dur)
{
    this->duration = dur;
    this->startTime = start;
}

void Animation::addItem(void *var, double startval, double endval)
{
    AnimationItem *myitem = new AnimationItem;
    myitem->initialize(var, startval, endval);
    this->items.push_back(myitem);
}

int Animation::update(double currentTime)
{
    std::list<AnimationItem *>::iterator myitem;
    bool complete = false;

    double pct_elapsed = (currentTime - this->startTime)/this->duration;
    if (pct_elapsed >= 1)
    {
        complete = true;
        pct_elapsed = 1;
    }

    for (myitem = this->items.begin(); myitem != this->items.end(); myitem++)
    {
        (*myitem)->update(pct_elapsed);
    }

    return complete;
}

AnimationItem::AnimationItem()
{
}

AnimationItem::~AnimationItem()
{
}

void AnimationItem::initialize(void *var, double startval, double endval)
{
    this->variable = (double *)var;
    this->startValue = startval;
    this->deltaValue = endval - startval;
}

void AnimationItem::update(double pct_elapsed)
{
    *(this->variable) = this->startValue + (pct_elapsed * this->deltaValue);
}
