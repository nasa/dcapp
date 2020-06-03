#ifndef _ANIMATION_HH_
#define _ANIMATION_HH_

#include <list>
#include "variables.hh"

class AnimationItem
{
    public:
        AnimationItem();
        virtual ~AnimationItem();

        void initialize(Variable *, double);
        void update(double);

    private:
        Variable *var;
        double startValue;
        double deltaValue;
};

class Animation
{
    public:
        Animation();
        virtual ~Animation();

        void initialize(double, double);
        void addItem(Variable *, double);
        int update(double);

    private:
        double duration;
        double startTime;
        std::list<AnimationItem *> items;
};

#endif
