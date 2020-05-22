#include "basicutils/stringutils.hh"
#include "nodes.hh"
#include "animate.hh"

extern appdata AppData;

dcAnimate::dcAnimate(dcParent *myparent) : duration(1)
{
    myparent->addChild(this);
}

void dcAnimate::setDuration(const char *inval)
{
    if (inval) duration = StringToDecimal(inval, 1);
}

void dcAnimate::handleEvent(void)
{
    AppData.events.push_back(this);
}

void dcAnimate::updateData(void)
{
    Animation *anim = new Animation;
    anim->initialize(AppData.master_timer->getSeconds(), duration);
    AppData.animators.push_back(anim);
    processAnimation(anim);
}
