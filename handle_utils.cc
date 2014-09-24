#include <stdlib.h>
#include <string.h>
#include "nodes.hh"
#include "trickio.hh"
#include "edgeio.hh"
#include "string_utils.hh"

extern void UpdateDisplay(void);

void ProcessEventList(struct node *);
void UpdateValue(struct node *);
int CheckCondition(struct node *);


void ProcessEventList(struct node *list)
{
    struct node *sublist;

    for (sublist = list; sublist != NULL; sublist = sublist->p_next)
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
        case FLOAT:
            return *(float *)val;
        case INTEGER:
            return (float)(*(int *)val);
        case STRING:
            return strtof((char *)val, NULL);
    }
    return 0;
}

int getIntegerVal(int type, void *val)
{
    switch (type)
    {
        case FLOAT:
            return (int)(*(float *)val);
        case INTEGER:
            return *(int *)val;
        case STRING:
            return strtol((char *)val, NULL, 10);
    }
    return 0;
}

int CheckConditionLogic(int opspec, int datatype1, void *val1, int datatype2, void *val2)
{
    int eval = 0;

    switch (datatype1)
    {
        case FLOAT:
            switch (opspec)
            {
                case Simple:
                    if (getFloatVal(datatype1, val1)) eval = 1;
                    break;
                case IfEquals:
                    if (getFloatVal(datatype1, val1) == getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfNotEquals:
                    if (getFloatVal(datatype1, val1) != getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfGreaterThan:
                    if (getFloatVal(datatype1, val1) > getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfLessThan:
                    if (getFloatVal(datatype1, val1) < getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfGreaterOrEquals:
                    if (getFloatVal(datatype1, val1) >= getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfLessOrEquals:
                    if (getFloatVal(datatype1, val1) <= getFloatVal(datatype2, val2)) eval = 1;
                    break;
            }
            break;
        case INTEGER:
            switch (opspec)
            {
                case Simple:
                    if (getIntegerVal(datatype1, val1)) eval = 1;
                    break;
                case IfEquals:
                    if (getIntegerVal(datatype1, val1) == getIntegerVal(datatype2, val2)) eval = 1;
                    break;
                case IfNotEquals:
                    if (getIntegerVal(datatype1, val1) != getIntegerVal(datatype2, val2)) eval = 1;
                    break;
                case IfGreaterThan:
                    if (getIntegerVal(datatype1, val1) > getIntegerVal(datatype2, val2)) eval = 1;
                    break;
                case IfLessThan:
                    if (getIntegerVal(datatype1, val1) < getIntegerVal(datatype2, val2)) eval = 1;
                    break;
                case IfGreaterOrEquals:
                    if (getIntegerVal(datatype1, val1) >= getIntegerVal(datatype2, val2)) eval = 1;
                    break;
                case IfLessOrEquals:
                    if (getIntegerVal(datatype1, val1) <= getIntegerVal(datatype2, val2)) eval = 1;
                    break;
            }
            break;
        case STRING:
            switch (opspec)
            {
                case Simple:
                    if (BoolStrToInt((char *)val1, 0)) eval = 1;
                    break;
                case IfEquals:
                    if (!strcmp((char *)val1, (char *)val2)) eval = 1;
                    break;
                case IfNotEquals:
                    if (strcmp((char *)val1, (char *)val2)) eval = 1;
                    break;
                case IfGreaterThan:
                    if (getFloatVal(datatype1, val1) > getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfLessThan:
                    if (getFloatVal(datatype1, val1) < getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfGreaterOrEquals:
                    if (getFloatVal(datatype1, val1) >= getFloatVal(datatype2, val2)) eval = 1;
                    break;
                case IfLessOrEquals:
                    if (getFloatVal(datatype1, val1) <= getFloatVal(datatype2, val2)) eval = 1;
                    break;
            }
            break;
    }

    return eval;
}

int CheckCondition(struct node *mynode)
{
    return CheckConditionLogic(mynode->object.cond.opspec,
                               mynode->object.cond.datatype1,
                               mynode->object.cond.val1,
                               mynode->object.cond.datatype2,
                               mynode->object.cond.val2);
}

void UpdateValueLogic(int optype, int vartype, void *var, int valtype, void *val, int mintype, void *min, int maxtype, void *max)
{
    switch (vartype)
    {
        case FLOAT:
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
        case INTEGER:
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
        case STRING:
            switch (valtype)
            {
                case STRING:
                    if (optype == Equals)
                        strcpy((char *)var, (char *)val);
                    break;
            }
            break;
    }

    trickio_forcewrite(var);
    edgeio_forcewrite(var);
}

void UpdateValue(struct node *mynode)
{
    UpdateValueLogic(mynode->object.modval.optype,
                     mynode->object.modval.datatype1, mynode->object.modval.var,
                     mynode->object.modval.datatype2, mynode->object.modval.val,
                     mynode->object.modval.mindatatype, mynode->object.modval.min,
                     mynode->object.modval.maxdatatype, mynode->object.modval.max);
}
