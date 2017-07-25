#include "nodes.hh"
#include "string_utils.hh"
#include "animate.hh"

// TODO: put in a centralized header file:
extern std::list<dcObject *> events;
extern appdata AppData;


dcAnimate::dcAnimate(dcParent *myparent) : duration(1)
{
    myparent->addChild(this);
}

void dcAnimate::setDuration(const char *inval)
{
    if (inval) duration = StrToFloat(inval, 1);
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
