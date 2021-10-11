#ifndef _MOUSEEVENT_HH_
#define _MOUSEEVENT_HH_

#include "geometric.hh"
#include "parent.hh"

class dcMouseEvent : public dcGeometric
{
    public:
        dcMouseEvent(dcParent *);
        virtual ~dcMouseEvent();

        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void processPreCalculations();
        void processPostCalculations();

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        bool selected;
};

#endif
