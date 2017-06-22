#ifndef _KEYBOARDEVENT_HH_
#define _KEYBOARDEVENT_HH_

#include "object.hh"
#include "parent.hh"

class dcKeyboardEvent : public dcObject
{
    public:
        dcKeyboardEvent(char);
        virtual ~dcKeyboardEvent();

        void handleKeyboard(char);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        char mykey; // TODO: better to have a key list
};

#endif
