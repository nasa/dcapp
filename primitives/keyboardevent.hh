#ifndef _KEYBOARDEVENT_HH_
#define _KEYBOARDEVENT_HH_

#include "object.hh"
#include "parent.hh"

class dcKeyboardEvent : public dcObject
{
    public:
        dcKeyboardEvent(dcParent *);
        dcKeyboardEvent(dcParent *, const char *);
        dcKeyboardEvent(dcParent *, const char *, const char *);
        virtual ~dcKeyboardEvent();
        void coreConstructor(dcParent *);
        void setKey(const char *);
        void setKeyAscii(const char *);
        void handleKeyPress(char);
        void handleKeyRelease(char);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        char mykey; // TODO: better to have a key list
};

#endif
