#ifndef _OBJECT_HH_
#define _OBJECT_HH_

#include "values.hh"
#include "animation.hh"

class dcObject
{
    public:
        dcObject() : parent(0x0) { };
        virtual ~dcObject() { };

        virtual void draw(void) { };
        virtual void handleKeyPress(char) { };
        virtual void handleKeyRelease(char) { };
        virtual void handleMousePress(double, double) { };
        virtual void handleMouseRelease(void) { };
        virtual void handleBezelPress(int) { };
        virtual void handleBezelRelease(int) { };
        virtual void handleEvent(void) { };
        virtual void handleMouseMotion(double, double) { };
        virtual void updateData(void) { };
        virtual void updateStreams(unsigned) { };
        virtual void processAnimation(Animation *) { };
        virtual void setParent(dcObject *parentid) { parent = parentid; };
        virtual dcObject *getParent(void) { return parent; };
        virtual Value *getContainerWidth(void) { return parent->getContainerWidth(); };
        virtual Value *getContainerHeight(void) { return parent->getContainerHeight(); };

    private:
        dcObject *parent;
};

#endif
