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

void Animation::addItem(Variable *var, double endval)
{
    AnimationItem *myitem = new AnimationItem;
    myitem->initialize(var, endval);
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

void AnimationItem::initialize(Variable *varin, double endval)
{
    this->var = varin;
    this->startValue = this->var->getDecimal();
    this->deltaValue = endval - this->startValue;
}

void AnimationItem::update(double pct_elapsed)
{
    this->var->setToDecimal(this->startValue + (pct_elapsed * this->deltaValue));
}
