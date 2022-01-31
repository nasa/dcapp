#include <cmath>

#include "app_data.hh"
#include "blink.hh"

extern appdata AppData;

dcBlink::dcBlink(dcParent *myparent, const std::string &frequency, const std::string &dutyCycle)
{
    myparent->addChild(this);
    period = 1 / ( getValue(frequency)->getDecimal() );
    onTime = period * getValue(dutyCycle)->getDecimal();
    duration = -1;
    remainingDuration = 0;
    blinkState = true;

    childList = new dcParent;
    childList->setParent(this);

    lastFrameTime = AppData.master_timer->getSeconds();
}

dcBlink::~dcBlink()
{
    return;
}

void dcBlink::setDuration(const std::string &inval)
{
    if (!inval.empty()) duration = getValue(inval)->getDecimal();
}

void dcBlink::setFnStartBlink(const std::string &inval)
{
    if (!inval.empty()) fnStartBlink = getValue(inval);
    prevStartBlinkState = fnStartBlink->getInteger();
}

void dcBlink::processPreCalculations()
{
    childList->processPreCalculations();
}

void dcBlink::processPostCalculations()
{
    childList->processPostCalculations();
}

void dcBlink::draw(void)
{
    int currStartBlinkState = fnStartBlink->getInteger();
    if (currStartBlinkState != prevStartBlinkState) {

        // duration < 0 = nonstop, > 0 = set duration
        if ( duration < 0 ) {
            remainingDuration = 1 - remainingDuration;  // 0 = not running, 1 = running
            blinkState = true;
        } else {
            remainingDuration = duration;
        }

        prevStartBlinkState = currStartBlinkState;
    }

    if ( remainingDuration > 0 ) {

        if ( fmod(AppData.master_timer->getSeconds(), period) > onTime ) {
            blinkState = false;
        } else {
            blinkState = true;
        }

        if ( duration > 0 ) {
            remainingDuration -= AppData.master_timer->getSeconds() - lastFrameTime;
            if ( remainingDuration <= 0 ) {
                blinkState = true;
            }
        }
    }

    if (blinkState) childList->draw();
    lastFrameTime = AppData.master_timer->getSeconds();
}
