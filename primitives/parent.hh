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
        void handleMouseMotion(double, double);
        void updateData(void);
        void updateStreams(unsigned);
        void processAnimation(Animation *);
        void processPreCalculations();
        void processPostCalculations();

        void addChild(dcObject *);

        std::list<dcObject *> children;
};

#endif
