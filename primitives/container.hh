#ifndef _CONTAINER_HH_
#define _CONTAINER_HH_

#include "geometric.hh"
#include "parent.hh"

class dcContainer : public dcParent, public dcGeometric
{
    public:
        dcContainer(dcParent *);
        void setSize(const char *, const char *);
        void setVirtualSize(const char *, const char *);
        void completeInitialization(void);
        void draw(void);
        void handleMousePress(float, float);

// TODO: why the heck to I need these?
void handleKeyPress(char key) { dcParent::handleKeyPress(key); };
void handleKeyRelease(char key) { dcParent::handleKeyRelease(key); };
void handleMouseRelease(void) { dcParent::handleMouseRelease(); };
void handleBezelPress(int key) { dcParent::handleBezelPress(key); };
void handleBezelRelease(int key) { dcParent::handleBezelRelease(key); };
void handleEvent(void) { dcParent::handleEvent(); };
void updateData(void) { dcParent::updateData(); };
void updateStreams(unsigned passcount) { dcParent::updateStreams(passcount); };
void processAnimation(Animation *anim) { dcParent::processAnimation(anim); };

        float *getContainerWidth(void);
        float *getContainerHeight(void);

    private:
        float *vwidth;
        float *vheight;
};

#endif
