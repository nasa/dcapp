#ifndef _CONTAINER_HH_
#define _CONTAINER_HH_

#include "parent.hh"

class dcContainer : public dcParent
{
    public:
        dcContainer(float *, float *, float *, float *, float *, float *, float *, float *, unsigned, unsigned, float *);

        void completeInitialization(void);
        void draw(void);
        void handleMousePress(float, float);
        float *getContainerWidth(void);
        float *getContainerHeight(void);

    private:
        float *vwidth;
        float *vheight;
        float *x;
        float *y;
        float *w;
        float *h;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        float *rotate;
};

#endif
