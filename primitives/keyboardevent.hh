#ifndef _KEYBOARDEVENT_HH_
#define _KEYBOARDEVENT_HH_

#include <string>
#include "object.hh"
#include "parent.hh"

class dcKeyboardEvent : public dcObject
{
    public:
        dcKeyboardEvent(dcParent *);
        dcKeyboardEvent(dcParent *, const std::string &);
        dcKeyboardEvent(dcParent *, const std::string &, const std::string &);
        virtual ~dcKeyboardEvent();
        void coreConstructor(dcParent *);
        void setKey(const std::string &);
        void setKeyAscii(const std::string &);
        void handleKeyPress(char);
        void handleKeyRelease(char);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        char mykey; // TODO: better to have a key list
};

#endif
