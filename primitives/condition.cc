#include "condition.hh"

// TODO: put in a centralized header file:
extern bool CheckConditionLogic(int, int, const void *, int, const void *);

dcCondition::dcCondition(int spec, int dtype1, void *v1, int dtype2, void *v2)
{
    opspec = spec;
    datatype1 = dtype1;
    datatype2 = dtype2;
    val1 = v1;
    val2 = v2;
    TrueList = new dcParent;
    FalseList = new dcParent;
}

dcCondition::~dcCondition()
{
    delete TrueList;
    delete FalseList;
}

void dcCondition::draw(void)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->draw();
    else
        FalseList->draw();
}

void dcCondition::handleKeyPress(char key)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->handleKeyPress(key);
    else
        FalseList->handleKeyPress(key);
}

void dcCondition::handleKeyRelease(char key)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->handleKeyRelease(key);
    else
        FalseList->handleKeyRelease(key);
}

void dcCondition::handleMousePress(float x, float y)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->handleMousePress(x, y);
    else
        FalseList->handleMousePress(x, y);
}

void dcCondition::handleMouseRelease(void)
{
    // check both lists in case the press was done while the condition was in a different state
    TrueList->handleMouseRelease();
    FalseList->handleMouseRelease();
}

void dcCondition::handleBezelPress(int key)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->handleBezelPress(key);
    else
        FalseList->handleBezelPress(key);
}

void dcCondition::handleBezelRelease(int key)
{
    // check both lists in case the press was done while the condition was in a different state
    TrueList->handleBezelRelease(key);
    FalseList->handleBezelRelease(key);
}

void dcCondition::handleEvent(void)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->handleEvent();
    else
        FalseList->handleEvent();
}

void dcCondition::updateStreams(unsigned passcount)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->updateStreams(passcount);
    else
        FalseList->updateStreams(passcount);
}

void dcCondition::processAnimation(Animation *anim)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->processAnimation(anim);
    else
        FalseList->processAnimation(anim);
}
