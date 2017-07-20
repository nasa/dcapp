#include "nodes.hh"
#include "animate.hh"

// TODO: put in a centralized header file:
extern std::list<dcObject *> events;
extern appdata AppData;


dcAnimate::dcAnimate(float inval)
{
    duration = inval;
}

void dcAnimate::handleEvent(void)
{
    events.push_back(this);
}

void dcAnimate::updateData(void)
{
    Animation *anim = new Animation;
    anim->initialize(AppData.master_timer->getSeconds(), duration);
    AppData.animators.push_back(anim);
    processAnimation(anim);
}
