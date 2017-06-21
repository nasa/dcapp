#ifndef _KEYBOARDEVENT_HH_
#define _KEYBOARDEVENT_HH_

#include "object.hh"
#include "parent.hh"

class dcKeyboardEvent : public dcObject
{
    public:
        dcKeyboardEvent(char);
        virtual ~dcKeyboardEvent();

//        virtual void draw(void) { };
        void handleKeyboard(char);
//        virtual void handleMousePress(float, float) { };
//        virtual void handleMouseRelease(void) { };

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        char mykey; // better to have a key list
};

#endif
