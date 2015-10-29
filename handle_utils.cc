#include <stdlib.h>
#include <string.h>
#include "nodes.hh"
#include "string_utils.hh"
#include "animation.hh"

extern void UpdateDisplay(void);

void ProcessEventList(struct node *);
void UpdateValue(struct node *);
void UpdateValueLogic(int, int, void *, int, void *, int, void *, int, void *);
bool CheckCondition(struct node *);

extern appdata AppData;


static void ProcessAnimationList(Animation *animator, struct node *list)
{
    struct node *sublist;
    float endval;

    for (sublist = list; sublist; sublist = sublist->p_next)
    {
        switch (sublist->info.type)
        {
            case Condition:
                if (CheckCondition(sublist))
                    ProcessAnimationList(animator, sublist->object.cond.TrueList);
                else
                    ProcessAnimationList(animator, sublist->object.cond.FalseList);
                break;
            case SetValue:
                if (sublist->object.modval.datatype1 == FLOAT_TYPE)
                {
                    UpdateValueLogic(sublist->object.modval.optype,
                                     FLOAT_TYPE, (void *)&endval,
                                     sublist->object.modval.datatype2, sublist->object.modval.val,
                                     sublist->object.modval.mindatatype, sublist->object.modval.min,
                                     sublist->object.modval.maxdatatype, sublist->object.modval.max);
                    animator->addItem(sublist->object.modval.var, *(float *)(sublist->object.modval.var), endval);
                }
                break;
            default:
                break;
        }
    }
}

void ProcessEventList(struct node *list)
{
    struct node *sublist;
    Animation *animobj;

    for (sublist = list; sublist; sublist = sublist->p_next)
    {
        switch (sublist->info.type)
        {
            case Condition:
                if (CheckCondition(sublist))
                    ProcessEventList(sublist->object.cond.TrueList);
                else
                    ProcessEventList(sublist->object.cond.FalseList);
                break;
            case SetValue:
                UpdateValue(sublist);
                break;
            case Animate:
                animobj = new Animation;
                animobj->initialize(AppData.master_timer->getSeconds(), sublist->object.anim.duration);
                AppData.animators.push_back(animobj);
                ProcessAnimationList(animobj, sublist->object.anim.SubList);
                break;
            default:
                break;
        }
    }
    UpdateDisplay();
}

float getFloatVal(int type, void *val)
{
    switch (type)
    {
        case FLOAT_TYPE:
            return *(float *)val;
        case INTEGER_TYPE:
            return (float)(*(int *)val);
        case STRING_TYPE:
            return strtof((char *)val, 0x0);
    }
    return 0;
}

int getIntegerVal(int type, void *val)
{
    switch (type)
    {
        case FLOAT_TYPE:
            return (int)(*(float *)val);
        case INTEGER_TYPE:
            return *(int *)val;
        case STRING_TYPE:
            return strtol((char *)val, 0x0, 10);
    }
    return 0;
}

bool CheckConditionLogic(int opspec, int datatype1, void *val1, int datatype2, void *val2)
{
    int eval = false;

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

bool CheckCondition(struct node *mynode)
{
    return CheckConditionLogic(mynode->object.cond.opspec,
                               mynode->object.cond.datatype1,
                               mynode->object.cond.val1,
                               mynode->object.cond.datatype2,
                               mynode->object.cond.val2);
}

void UpdateValueLogic(int optype, int vartype, void *var, int valtype, void *val, int mintype, void *min, int maxtype, void *max)
{
    std::list<CommModule *>::iterator commitem;

    switch (vartype)
    {
        case FLOAT_TYPE:
            switch (optype)
            {
                case PlusEquals:
                    *(float *)var += getFloatVal(valtype, val);
                    break;
                case MinusEquals:
                    *(float *)var -= getFloatVal(valtype, val);
                    break;
                default:
                    *(float *)var = getFloatVal(valtype, val);
            }
            if (min)
            {
                float minval = getFloatVal(mintype, min);
                if (*(float *)var < minval) *(float *)var = minval;
            }
            if (max)
            {
                float maxval = getFloatVal(maxtype, max);
                if (*(float *)var > maxval) *(float *)var = maxval;
            }
            break;
        case INTEGER_TYPE:
            switch (optype)
            {
                case PlusEquals:
                    *(int *)var += getIntegerVal(valtype, val);
                    break;
                case MinusEquals:
                    *(int *)var -= getIntegerVal(valtype, val);
                    break;
                default:
                    *(int *)var = getIntegerVal(valtype, val);
            }
            if (min)
            {
                int minval = getIntegerVal(mintype, min);
                if (*(int *)var < minval) *(int *)var = minval;
            }
            if (max)
            {
                int maxval = getIntegerVal(maxtype, max);
                if (*(int *)var > maxval) *(int *)var = maxval;
            }
            break;
        case STRING_TYPE:
            switch (valtype)
            {
                case STRING_TYPE:
                    if (optype == Equals)
                        strcpy((char *)var, (char *)val);
                    break;
            }
            break;
    }

    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        (*commitem)->flagAsChanged(var);
    }
}

void UpdateValue(struct node *mynode)
{
    UpdateValueLogic(mynode->object.modval.optype,
                     mynode->object.modval.datatype1, mynode->object.modval.var,
                     mynode->object.modval.datatype2, mynode->object.modval.val,
                     mynode->object.modval.mindatatype, mynode->object.modval.min,
                     mynode->object.modval.maxdatatype, mynode->object.modval.max);
}
