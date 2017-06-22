#ifndef _MOUSEEVENT_HH_
#define _MOUSEEVENT_HH_

#include "object.hh"
#include "parent.hh"

class dcMouseEvent : public dcObject
{
    public:
        dcMouseEvent(int);
        virtual ~dcMouseEvent();

        void handleMousePress(int);
        void handleMouseRelease(int);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        int mykey; // TODO: better to have a key list
        bool selected;
};

#endif
