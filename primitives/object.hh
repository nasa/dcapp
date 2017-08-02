#ifndef _OBJECT_HH_
#define _OBJECT_HH_

#include "animation.hh"

class dcObject
{
    public:
        dcObject() : parent(0x0) { };
        virtual ~dcObject() { };

        virtual void draw(void) { };
        virtual void handleKeyPress(char) { };
        virtual void handleKeyRelease(char) { };
        virtual void handleMousePress(float, float) { };
        virtual void handleMouseRelease(void) { };
        virtual void handleBezelPress(int) { };
        virtual void handleBezelRelease(int) { };
        virtual void handleEvent(void) { };
        virtual void updateData(void) { };
        virtual void updateStreams(unsigned) { };
        virtual void processAnimation(Animation *) { };
        virtual void setParent(dcObject *parentid) { parent = parentid; };
        virtual dcObject *getParent(void) { return parent; };
        virtual float *getContainerWidth(void) { return parent->getContainerWidth(); };
        virtual float *getContainerHeight(void) { return parent->getContainerHeight(); };

    private:
        dcObject *parent;
};

#endif