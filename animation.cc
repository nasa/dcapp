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

void Animation::initialize(float start, float dur)
{
    this->duration = dur;
    this->startTime = start;
}

void Animation::addItem(void *var, float startval, float endval)
{
    AnimationItem *myitem = new AnimationItem;
    myitem->initialize(var, startval, endval);
    this->items.push_back(myitem);
}

int Animation::update(float currentTime)
{
    std::list<AnimationItem *>::iterator myitem;
    bool complete = false;

    float pct_elapsed = (currentTime - this->startTime)/this->duration;
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

void AnimationItem::initialize(void *var, float startval, float endval)
{
    this->variable = (float *)var;
    this->startValue = startval;
    this->deltaValue = endval - startval;
}

void AnimationItem::update(float pct_elapsed)
{
    *(this->variable) = this->startValue + (pct_elapsed * this->deltaValue);
}
