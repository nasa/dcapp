#include <cstdlib>
#include <cstring>
#include <list>
#include "primitives/primitives.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "animation.hh"

extern void UpdateDisplay(void);

extern appdata AppData;

std::list<dcObject *> events;

void ProcessEvents(void)
{
    if (!events.empty())
    {
        std::list<dcObject *>::iterator event;
        for (event = events.begin(); event != events.end(); event++)
        {
            (*event)->updateData();
        }
        events.clear();
        UpdateDisplay();
    }
}

float getFloatVal(int type, const void *val)
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

int getIntegerVal(int type, const void *val)
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

bool CheckConditionLogic(int opspec, int datatype1, const void *val1, int datatype2, const void *val2)
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
