#ifndef _BEZELEVENT_HH_
#define _BEZELEVENT_HH_

#include <string>
#include "object.hh"
#include "parent.hh"

class dcBezelEvent : public dcObject
{
    public:
        dcBezelEvent(dcParent *);
        dcBezelEvent(dcParent *, const std::string &);
        virtual ~dcBezelEvent();
        void coreConstructor(dcParent *);
        void setKey(const std::string &);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void processPreCalculations();
        void processPostCalculations();

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        int mykey; // TODO: better to have a key list
        bool selected;
};

#endif
