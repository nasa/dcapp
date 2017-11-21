#ifndef _ANIMATE_HH_
#define _ANIMATE_HH_

#include "parent.hh"

class dcAnimate : public dcParent
{
    public:
        dcAnimate(dcParent *);
        void setDuration(const char *);
        void handleEvent(void);
        void updateData(void);

    private:
        double duration;
};

#endif
