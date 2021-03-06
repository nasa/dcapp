#ifndef _ANIMATE_HH_
#define _ANIMATE_HH_

#include <string>
#include "parent.hh"

class dcAnimate : public dcParent
{
    public:
        dcAnimate(dcParent *);
        void setDuration(const std::string &);
        void handleEvent(void);
        void updateData(void);

    private:
        double duration;
};

#endif
