#include "nodes.hh"
#include "types.hh"
#include "condition.hh"

// TODO: put in a centralized header file:
extern int get_data_type(const char *);
extern void *getVariablePointer(int, const char *);
extern bool CheckConditionLogic(int, int, const void *, int, const void *);

dcCondition::dcCondition(dcParent *myparent, const char *inspec, const char *inval1, const char *inval2)
{
// TODO: abort initialization if inval1 and inval2 are both nullptr
    coreConstructor(myparent);

    opspec = Simple;
    if (inspec)
    {
        if (!strcasecmp(inspec, "eq")) opspec = IfEquals;
        else if (!strcasecmp(inspec, "ne")) opspec = IfNotEquals;
        else if (!strcasecmp(inspec, "gt")) opspec = IfGreaterThan;
        else if (!strcasecmp(inspec, "lt")) opspec = IfLessThan;
        else if (!strcasecmp(inspec, "ge")) opspec = IfGreaterOrEquals;
        else if (!strcasecmp(inspec, "le")) opspec = IfLessOrEquals;
    }

    datatype1 = get_data_type(inval1);
    datatype2 = get_data_type(inval2);
    if (datatype1 == UNDEFINED_TYPE && datatype2 == UNDEFINED_TYPE)
    {
        datatype1 = STRING_TYPE;
        datatype2 = STRING_TYPE;
    }
    else if (datatype1 == UNDEFINED_TYPE) datatype1 = datatype2;
    else if (datatype2 == UNDEFINED_TYPE) datatype2 = datatype1;

    if (inval1) val1 = getVariablePointer(datatype1, inval1);
    else val1 = getVariablePointer(datatype1, "1");
    if (inval2) val2 = getVariablePointer(datatype2, inval2);
    else val2 = getVariablePointer(datatype2, "1");
}

dcCondition::~dcCondition()
{
    delete TrueList;
    delete FalseList;
}

void dcCondition::coreConstructor(dcParent *myparent)
{
    myparent->addChild(this);
    TrueList = new dcParent;
    FalseList = new dcParent;
    TrueList->setParent(this);
    FalseList->setParent(this);
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
