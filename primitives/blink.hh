#ifndef _BLINK_HH_
#define _BLINK_HH_

#include "xml_data.hh"
#include "values.hh"
#include "object.hh"
#include "parent.hh"

class dcBlink : public dcObject
{
    public:
        dcBlink(dcParent *, const std::string &, const std::string &);
        virtual ~dcBlink();

        void setDuration(const std::string &);
        void setFnStartBlink(const std::string &);

        void processPreCalculations();
        void processPostCalculations();
        void draw(void);

        dcParent *childList;

    private:
        Value* fnStartBlink;        // function to start blinking
        int    prevStartBlinkState; // tied to fnStartBlink

        // all time variables in seconds
        float period;
        float onTime;
        float duration;

        float currentOnTime;
        float remainingDuration;
        float lastFrameTime;
        bool  blinkState;
};

#endif