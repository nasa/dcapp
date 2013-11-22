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
}

void UpdateValue(struct node *mynode)
{
    switch (mynode->object.modval.datatype1)
    {
        case FLOAT:
            switch (mynode->object.modval.datatype2)
            {
                case FLOAT:
                    *(float *)(mynode->object.modval.var) = *(float *)(mynode->object.modval.val);
                    break;
                case INTEGER:
                    *(float *)(mynode->object.modval.var) = (float)(*(int *)(mynode->object.modval.val));
                    break;
                case STRING:
                    *(float *)(mynode->object.modval.var) = strtof((char *)(mynode->object.modval.val), NULL);
                    break;
            }
            break;
        case INTEGER:
            switch (mynode->object.modval.datatype2)
            {
                case FLOAT:
                    *(int *)(mynode->object.modval.var) = (int)(*(float *)(mynode->object.modval.val));
                    break;
                case INTEGER:
                    *(int *)(mynode->object.modval.var) = *(int *)(mynode->object.modval.val);
                    break;
                case STRING:
                    *(int *)(mynode->object.modval.var) = strtol((char *)(mynode->object.modval.val), NULL, 10);
                    break;
            }
            *(int *)(mynode->object.modval.var) = *(int *)(mynode->object.modval.val);
            break;
        case STRING:
            strcpy((char *)(mynode->object.modval.var), (char *)(mynode->object.modval.val));
            break;
    }
    trickio_forcewrite(mynode->object.modval.var);
    edgeio_forcewrite(mynode->object.modval.var);
}
