#ifndef _CONTAINER_HH_
#define _CONTAINER_HH_

#include <string>
#include "values.hh"
#include "geometric.hh"
#include "parent.hh"

class dcContainer : public dcParent, public dcGeometric
{
    public:
        dcContainer(dcParent *);
        void setSize(const std::string &, const std::string &);
        void setVirtualSize(const std::string &, const std::string &);
        void draw(void);
        void handleMousePress(double, double);
        void handleMouseMotion(double, double);

// why the heck to I need these?
// answer: diamond problem, explicitly state which parent function to use
void handleKeyPress(char key) { dcParent::handleKeyPress(key); };
void handleKeyRelease(char key) { dcParent::handleKeyRelease(key); };
void handleMouseRelease(void) { dcParent::handleMouseRelease(); };
void handleBezelPress(int key) { dcParent::handleBezelPress(key); };
void handleBezelRelease(int key) { dcParent::handleBezelRelease(key); };
void handleEvent(void) { dcParent::handleEvent(); };
void updateData(void) { dcParent::updateData(); };
void updateStreams(unsigned passcount) { dcParent::updateStreams(passcount); };
void processAnimation(Animation *anim) { dcParent::processAnimation(anim); };
void processPreCalculations(void) { dcParent::processPreCalculations(); };
void processPostCalculations(void) { dcParent::processPostCalculations(); };

        Value *getContainerWidth(void);
        Value *getContainerHeight(void);

    private:
        Value *vwidth;
        Value *vheight;
};

#endif
