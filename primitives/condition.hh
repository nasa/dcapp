#ifndef _CONDITION_HH_
#define _CONDITION_HH_

#include "animation.hh"
#include "object.hh"
#include "parent.hh"

class dcCondition : public dcObject
{
    public:
        dcCondition(dcParent *, const char *, const char *, const char *);
        virtual ~dcCondition();
        void draw(void);
        void handleKeyPress(char);
        void handleKeyRelease(char);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void handleEvent(void);
        void handleMouseMotion(double, double);
        void updateStreams(unsigned);
        void processAnimation(Animation *);
        bool checkCondition(void);

        dcParent *TrueList;
        dcParent *FalseList;

    private:
        int opspec;
        int datatype1;
        int datatype2;
        void *val1;
        void *val2;
};

#endif
