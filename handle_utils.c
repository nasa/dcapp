#include <stdlib.h>
#include <string.h>
#include "nodes.h"
#include "trickio.h"
#include "edgeio.h"

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

int CheckCondition(struct node *mynode)
{
#if 0
    if (mynode->object.cond.opspec == Simple)
    {
        return 1;
    }
    else
    {
#endif
        if (mynode->object.cond.datatype1 != mynode->object.cond.datatype2) return 0;

        switch (mynode->object.cond.datatype1)
        {
            case FLOAT:
                if (*(float *)(mynode->object.cond.val1) == *(float *)(mynode->object.cond.val2)) return 1;
                else return 0;
                break;
            case INTEGER:
                if (*(int *)(mynode->object.cond.val1) == *(int *)(mynode->object.cond.val2)) return 1;
                else return 0;
                break;
            case STRING:
                if (strcmp((char *)(mynode->object.cond.val1), (char *)(mynode->object.cond.val2))) return 0;
                else return 1;
                break;
        }

        return 0;
//    }
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

void UpdateValue(struct node *mynode)
{
    switch (mynode->object.modval.datatype1)
    {
        case FLOAT:
            switch (mynode->object.modval.optype)
            {
                case PlusEquals:
                    *(float *)(mynode->object.modval.var) += getFloatVal(mynode->object.modval.datatype2, mynode->object.modval.val);
                    break;
                case MinusEquals:
                    *(float *)(mynode->object.modval.var) -= getFloatVal(mynode->object.modval.datatype2, mynode->object.modval.val);
                    break;
                default:
                    *(float *)(mynode->object.modval.var) = getFloatVal(mynode->object.modval.datatype2, mynode->object.modval.val);
            }
            if (mynode->object.modval.min)
            {
                float minval = getFloatVal(mynode->object.modval.mindatatype, mynode->object.modval.min);
                if (*(float *)(mynode->object.modval.var) < minval) *(float *)(mynode->object.modval.var) = minval;
            }
            if (mynode->object.modval.max)
            {
                float maxval = getFloatVal(mynode->object.modval.maxdatatype, mynode->object.modval.max);
                if (*(float *)(mynode->object.modval.var) > maxval) *(float *)(mynode->object.modval.var) = maxval;
            }
            break;
        case INTEGER:
            switch (mynode->object.modval.optype)
            {
                case PlusEquals:
                    *(int *)(mynode->object.modval.var) += getIntegerVal(mynode->object.modval.datatype2, mynode->object.modval.val);
                    break;
                case MinusEquals:
                    *(int *)(mynode->object.modval.var) -= getIntegerVal(mynode->object.modval.datatype2, mynode->object.modval.val);
                    break;
                default:
                    *(int *)(mynode->object.modval.var) = getIntegerVal(mynode->object.modval.datatype2, mynode->object.modval.val);
            }
            if (mynode->object.modval.min)
            {
                int minval = getIntegerVal(mynode->object.modval.mindatatype, mynode->object.modval.min);
                if (*(int *)(mynode->object.modval.var) < minval) *(int *)(mynode->object.modval.var) = minval;
            }
            if (mynode->object.modval.max)
            {
                int maxval = getIntegerVal(mynode->object.modval.maxdatatype, mynode->object.modval.max);
                if (*(int *)(mynode->object.modval.var) > maxval) *(int *)(mynode->object.modval.var) = maxval;
            }
            break;
        case STRING:
            switch (mynode->object.modval.datatype2)
            {
                case STRING:
                    if (mynode->object.modval.optype == Equals)
                        strcpy((char *)(mynode->object.modval.var), (char *)(mynode->object.modval.val));
                    break;
            }
            break;
    }

    trickio_forcewrite(mynode->object.modval.var);
    edgeio_forcewrite(mynode->object.modval.var);
}
