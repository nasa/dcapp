#include <cstring>
#include <string>
#include "string_utils.hh"
#include "valuedata.hh"
#include "varlist.hh"
#include "constants.hh"
#include "condition.hh"

enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals };
enum { Neither, LHS, RHS };

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

    if (inval1) val1 = getValue(inval1);
    else val1 = getConstantFromBoolean(true);

    if (inval2) val2 = getValue(inval2);
    else val2 = getConstantFromBoolean(true);

    if (opspec != Simple)
    {
        if (val1->isVariable()) dynvar = LHS;
        else if (val2->isVariable()) dynvar = RHS;
        else dynvar = Neither;
    }
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
    if (opspec == Simple) return val1->getBoolean();

    switch (dynvar)
    {
        case LHS:
            switch (opspec)
            {
                case IfNotEquals:
                    if (val1->compareToValue(*val2) != isEqual) return true; else return false;
                case IfGreaterThan:
                    if (val1->compareToValue(*val2) == isGreaterThan) return true; else return false;
                case IfLessThan:
                    if (val1->compareToValue(*val2) == isLessThan) return true; else return false;
                case IfGreaterOrEquals:
                    if (val1->compareToValue(*val2) != isLessThan) return true; else return false;
                case IfLessOrEquals:
                    if (val1->compareToValue(*val2) != isGreaterThan) return true; else return false;
                default:
                    if (val1->compareToValue(*val2) == isEqual) return true; else return false;
            }
        case RHS:
            // these look backwards, but it's because we're evaluating the comparison backwards
            switch (opspec)
            {
                case IfNotEquals:
                    if (val2->compareToValue(*val1) != isEqual) return true; else return false;
                case IfGreaterThan:
                    if (val2->compareToValue(*val1) == isLessThan) return true; else return false;
                case IfLessThan:
                    if (val2->compareToValue(*val1) == isGreaterThan) return true; else return false;
                case IfGreaterOrEquals:
                    if (val2->compareToValue(*val1) != isGreaterThan) return true; else return false;
                case IfLessOrEquals:
                    if (val2->compareToValue(*val1) != isLessThan) return true; else return false;
                default:
                    if (val2->compareToValue(*val1) == isEqual) return true; else return false;
            }
        default:
            // evaluate as strings if both sides are Constants
            switch (opspec)
            {
                case IfNotEquals:
                    if (val1->getString() != val2->getString()) return true; else return false;
                case IfGreaterThan:
                    if (val1->getString() > val2->getString()) return true; else return false;
                case IfLessThan:
                    if (val1->getString() < val2->getString()) return true; else return false;
                case IfGreaterOrEquals:
                    if (val1->getString() >= val2->getString()) return true; else return false;
                case IfLessOrEquals:
                    if (val1->getString() <= val2->getString()) return true; else return false;
                default:
                    if (val1->getString() == val2->getString()) return true; else return false;
            }
    }
}
