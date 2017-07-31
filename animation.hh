#ifndef _ANIMATION_HH_
#define _ANIMATION_HH_

#include <list>

class AnimationItem
{
    public:
        AnimationItem();
        virtual ~AnimationItem();

        void initialize(void *, float, float);
        void update(float);

    private:
        float *variable;
        float startValue;
        float deltaValue;
};

class Animation
{
    public:
        Animation();
        virtual ~Animation();

        void initialize(float, float);
        void addItem(void *, float, float);
        int update(float);

    private:
        float duration;
        float startTime;
        std::list<AnimationItem *> items;
};

#endif
