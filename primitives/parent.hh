#ifndef _PARENT_HH_
#define _PARENT_HH_

#include <list>
#include "object.hh"

class dcParent : public dcObject
{
    public:
        dcParent() { };
        virtual ~dcParent();

        void handleKeyboard(char);
        void handleMousePress(float, float);
        void handleMouseRelease(void);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void updateData(void);
        void updateStreams(unsigned);
        void processAnimation(void);

        void addChild(dcObject *);

    protected:
        std::list<dcObject *> children;
};

#endif
