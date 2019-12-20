#include <cstring>
#include <string>
#include "string_utils.hh"
#include "types.hh"
#include "varlist.hh"
#include "condition.hh"

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

void dcCondition::handleMousePress(double x, double y)
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

void dcCondition::handleMouseMotion(double x, double y)
{
    if (checkCondition())
        TrueList->handleMouseMotion(x, y);
    else
        FalseList->handleMouseMotion(x, y);
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
        case DECIMAL_TYPE:
            switch (opspec)
            {
                case Simple:
                    if (getDecimalValue(datatype1, val1)) eval = true;
                    break;
                case IfEquals:
                    if (getDecimalValue(datatype1, val1) == getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfNotEquals:
                    if (getDecimalValue(datatype1, val1) != getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfGreaterThan:
                    if (getDecimalValue(datatype1, val1) > getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfLessThan:
                    if (getDecimalValue(datatype1, val1) < getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfGreaterOrEquals:
                    if (getDecimalValue(datatype1, val1) >= getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfLessOrEquals:
                    if (getDecimalValue(datatype1, val1) <= getDecimalValue(datatype2, val2)) eval = true;
                    break;
            }
            break;
        case INTEGER_TYPE:
            switch (opspec)
            {
                case Simple:
                    if (getIntegerValue(datatype1, val1)) eval = true;
                    break;
                case IfEquals:
                    if (getIntegerValue(datatype1, val1) == getIntegerValue(datatype2, val2)) eval = true;
                    break;
                case IfNotEquals:
                    if (getIntegerValue(datatype1, val1) != getIntegerValue(datatype2, val2)) eval = true;
                    break;
                case IfGreaterThan:
                    if (getIntegerValue(datatype1, val1) > getIntegerValue(datatype2, val2)) eval = true;
                    break;
                case IfLessThan:
                    if (getIntegerValue(datatype1, val1) < getIntegerValue(datatype2, val2)) eval = true;
                    break;
                case IfGreaterOrEquals:
                    if (getIntegerValue(datatype1, val1) >= getIntegerValue(datatype2, val2)) eval = true;
                    break;
                case IfLessOrEquals:
                    if (getIntegerValue(datatype1, val1) <= getIntegerValue(datatype2, val2)) eval = true;
                    break;
            }
            break;
        case STRING_TYPE:
            switch (opspec)
            {
                case Simple:
                    if (StringToBoolean(*(std::string *)val1)) eval = true;
                    break;
                case IfEquals:
                    if (getStringValue(datatype1, val1) == getStringValue(datatype2, val2)) eval = true;
                    break;
                case IfNotEquals:
                    if (getStringValue(datatype1, val1) != getStringValue(datatype2, val2)) eval = true;
                    break;
                case IfGreaterThan:
                    if (getDecimalValue(datatype1, val1) > getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfLessThan:
                    if (getDecimalValue(datatype1, val1) < getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfGreaterOrEquals:
                    if (getDecimalValue(datatype1, val1) >= getDecimalValue(datatype2, val2)) eval = true;
                    break;
                case IfLessOrEquals:
                    if (getDecimalValue(datatype1, val1) <= getDecimalValue(datatype2, val2)) eval = true;
                    break;
            }
            break;
    }

    return eval;
}
