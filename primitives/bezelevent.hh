#ifndef _BEZELEVENT_HH_
#define _BEZELEVENT_HH_

#include "object.hh"
#include "parent.hh"

class dcBezelEvent : public dcObject
{
    public:
        dcBezelEvent(int);
        virtual ~dcBezelEvent();

        void completeInitialization(void);
        void handleBezelPress(int);
        void handleBezelRelease(int);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        int mykey; // TODO: better to have a key list
        bool selected;
};

#endif
