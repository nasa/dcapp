#ifndef _PARENT_HH_
#define _PARENT_HH_

#include <list>
#include "animation.hh"
#include "object.hh"

class dcParent : public dcObject
{
    public:
        dcParent() { };
        virtual ~dcParent();

        void draw(void);
        void handleKeyPress(char);
        void handleKeyRelease(char);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void handleEvent(void);
        void updateData(void);
        void updateStreams(unsigned);
        void processAnimation(Animation *);

        void addChild(dcObject *);

    protected:
        std::list<dcObject *> children;
};

#endif
