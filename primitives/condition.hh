#ifndef _CONDITION_HH_
#define _CONDITION_HH_

#include "xml_data.hh"
#include "values.hh"
#include "animation.hh"
#include "object.hh"
#include "parent.hh"

class dcCondition : public dcObject
{
    public:
        dcCondition(dcParent *, const xmldata &, const xmldata &, const xmldata &);
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
        void processPreCalculations();
        void processPostCalculations();

        dcParent *TrueList;
        dcParent *FalseList;

    private:
        bool eval;
        unsigned opspec;
        unsigned dynvar;
        Value *val1;
        Value *val2;
};

#endif
