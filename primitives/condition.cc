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

void dcCondition::handleKeyboard(char key)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->handleKeyboard(key);
    else
        FalseList->handleKeyboard(key);
}

void dcCondition::updateData(void)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->updateData();
    else
        FalseList->updateData();
}

void dcCondition::processAnimation(void)
{
    if (CheckConditionLogic(opspec, datatype1, val1, datatype2, val2))
        TrueList->processAnimation();
    else
        FalseList->processAnimation();
}
