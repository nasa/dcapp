#ifndef _PARENT_HH_
#define _PARENT_HH_

#include <list>
#include "object.hh"

class dcParent : public dcObject
{
    public:
        dcParent() { };
        virtual ~dcParent();

        void draw(void);
        void handleKeyPress(char);
        void handleKeyRelease(char);
        void handleMousePress(float, float);
        void handleMouseRelease(void);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void handleEvent(void);
        void updateStreams(unsigned);
        void processAnimation(void);

        void addChild(dcObject *);

    protected:
        std::list<dcObject *> children;
};

#endif
