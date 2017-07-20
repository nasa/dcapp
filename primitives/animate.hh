#ifndef _ANIMATE_HH_
#define _ANIMATE_HH_

#include "parent.hh"

class dcAnimate : public dcParent
{
    public:
        dcAnimate(float);

        void handleEvent(void);
        void updateData(void);

    private:
        float duration;
};

#endif
