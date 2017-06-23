#ifndef _MOUSEEVENT_HH_
#define _MOUSEEVENT_HH_

#include "object.hh"
#include "parent.hh"

class dcMouseEvent : public dcObject
{
    public:
        dcMouseEvent(float *, float *, float *, float *, float *, float *, unsigned, unsigned);
        virtual ~dcMouseEvent();

        void handleMousePress(float, float);
        void handleMouseRelease(void);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        float *x;
        float *y;
        float *w;
        float *h;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        bool selected;
};

#endif
