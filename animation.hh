#ifndef _ANIMATION_HH_
#define _ANIMATION_HH_

#include <list>

class AnimationItem
{
    public:
        AnimationItem();
        virtual ~AnimationItem();

        void initialize(void *, double, double);
        void update(double);

    private:
        double *variable;
        double startValue;
        double deltaValue;
};

class Animation
{
    public:
        Animation();
        virtual ~Animation();

        void initialize(double, double);
        void addItem(void *, double, double);
        int update(double);

    private:
        double duration;
        double startTime;
        std::list<AnimationItem *> items;
};

#endif
