#include <cstring>
#include "string_utils.hh"
#include "types.hh"
#include "varlist.hh"
#include "condition.hh"

// TODO: put in a centralized header file:
extern int getIntegerVal(int, const void *);
extern float getFloatVal(int, const void *);

enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals };

dcCondition::dcCondition(dcParent *myparent, const char *inspec, const char *inval1, const char *inval2)
{
    // don't parent this object if inval1 and inval2 are both nullptr
    if (!inval1 && !inval2) return;

    // this object doesn't require a parent since it can be used during preprocessing
    if (myparent) myparent->addChild(this);
    TrueList = new dcParent;
    FalseList = new dcParent;
    TrueList->setParent(this);
    FalseList->setParent(this);

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

void dcCondition::draw(void)
{
    if (checkCondition())
        TrueList->draw();
    else
        FalseList->draw();
}

void dcCondition::handleKeyPress(char key)
{
    if (checkCondition())
        TrueList->handleKeyPress(key);
    else
        FalseList->handleKeyPress(key);
}

void dcCondition::handleKeyRelease(char key)
{
    if (checkCondition())
        TrueList->handleKeyRelease(key);
    else
        FalseList->handleKeyRelease(key);
}

void dcCondition::handleMousePress(float x, float y)
{
    if (checkCondition())
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
    if (checkCondition())
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
    if (checkCondition())
        TrueList->handleEvent();
    else
        FalseList->handleEvent();
}

void dcCondition::updateStreams(unsigned passcount)
{
    if (checkCondition())
        TrueList->updateStreams(passcount);
    else
        FalseList->updateStreams(passcount);
}

void dcCondition::processAnimation(Animation *anim)
{
    if (checkCondition())
        TrueList->processAnimation(anim);
    else
        FalseList->processAnimation(anim);
}

bool dcCondition::checkCondition(void)
{
    bool eval = false;

    switch (datatype1)
    {
        case FLOAT_TYPE:
            switch (opspec)
            {
                case Simple:
                    if (getFloatVal(datatype1, val1)) eval = true;
                    break;
                case IfEquals:
                    if (getFloatVal(datatype1, val1) == getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfNotEquals:
                    if (getFloatVal(datatype1, val1) != getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfGreaterThan:
                    if (getFloatVal(datatype1, val1) > getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfLessThan:
                    if (getFloatVal(datatype1, val1) < getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfGreaterOrEquals:
                    if (getFloatVal(datatype1, val1) >= getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfLessOrEquals:
                    if (getFloatVal(datatype1, val1) <= getFloatVal(datatype2, val2)) eval = true;
                    break;
            }
            break;
        case INTEGER_TYPE:
            switch (opspec)
            {
                case Simple:
                    if (getIntegerVal(datatype1, val1)) eval = true;
                    break;
                case IfEquals:
                    if (getIntegerVal(datatype1, val1) == getIntegerVal(datatype2, val2)) eval = true;
                    break;
                case IfNotEquals:
                    if (getIntegerVal(datatype1, val1) != getIntegerVal(datatype2, val2)) eval = true;
                    break;
                case IfGreaterThan:
                    if (getIntegerVal(datatype1, val1) > getIntegerVal(datatype2, val2)) eval = true;
                    break;
                case IfLessThan:
                    if (getIntegerVal(datatype1, val1) < getIntegerVal(datatype2, val2)) eval = true;
                    break;
                case IfGreaterOrEquals:
                    if (getIntegerVal(datatype1, val1) >= getIntegerVal(datatype2, val2)) eval = true;
                    break;
                case IfLessOrEquals:
                    if (getIntegerVal(datatype1, val1) <= getIntegerVal(datatype2, val2)) eval = true;
                    break;
            }
            break;
        case STRING_TYPE:
            switch (opspec)
            {
                case Simple:
                    if (StrToBool((char *)val1, false)) eval = true;
                    break;
                case IfEquals:
                    if (!strcmp((char *)val1, (char *)val2)) eval = true;
                    break;
                case IfNotEquals:
                    if (strcmp((char *)val1, (char *)val2)) eval = true;
                    break;
                case IfGreaterThan:
                    if (getFloatVal(datatype1, val1) > getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfLessThan:
                    if (getFloatVal(datatype1, val1) < getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfGreaterOrEquals:
                    if (getFloatVal(datatype1, val1) >= getFloatVal(datatype2, val2)) eval = true;
                    break;
                case IfLessOrEquals:
                    if (getFloatVal(datatype1, val1) <= getFloatVal(datatype2, val2)) eval = true;
                    break;
            }
            break;
    }

    return eval;
}
