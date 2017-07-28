#include <cstring>
#include "nodes.hh"
#include "types.hh"
#include "setvalue.hh"

// TODO: put in a centralized header file:
#include <list>
extern std::list<dcObject *> events;
extern void *getVariablePointer(int, const char *);
extern int get_data_type(const char *);
extern int getIntegerVal(int, const void *);
extern float getFloatVal(int, const void *);
extern appdata AppData;

dcSetValue::dcSetValue(dcParent *myparent, const char *invar, const char *inval)
:
optype(Equals), mindatatype(UNDEFINED_TYPE), maxdatatype(UNDEFINED_TYPE), min(0x0), max(0x0)
{
    datatype1 = get_data_type(invar);

    // don't parent this object if var isn't properly defined
    if (datatype1 == UNDEFINED_TYPE) return;

    var = getVariablePointer(datatype1, invar);

    datatype2 = get_data_type(inval);
    if (datatype2 == UNDEFINED_TYPE) datatype2 = datatype1;
    val = getVariablePointer(datatype2, inval);

    // this object doesn't require a parent since it can be used during preprocessing
    if (myparent) myparent->addChild(this);
}

void dcSetValue::setOperator(const char *opspec)
{
    if (!opspec) return;
    else if (!strcmp(opspec, "+=")) optype = PlusEquals;
    else if (!strcmp(opspec, "-=")) optype = MinusEquals;
}

void dcSetValue::setRange(const char *minspec, const char *maxspec)
{
    if (minspec)
    {
        mindatatype = get_data_type(minspec);
        if (mindatatype == UNDEFINED_TYPE) mindatatype = datatype1;
        min = getVariablePointer(mindatatype, minspec);
    }

    if (maxspec)
    {
        maxdatatype = get_data_type(maxspec);
        if (maxdatatype == UNDEFINED_TYPE) maxdatatype = datatype1;
        max = getVariablePointer(maxdatatype, maxspec);
    }
}

void dcSetValue::draw(void)
{
    calculateValue(optype, datatype1, var, datatype2, val, mindatatype, min, maxdatatype, max);
}

void dcSetValue::handleEvent(void)
{
    events.push_back(this);
}

void dcSetValue::updateData(void)
{
    calculateValue(optype, datatype1, var, datatype2, val, mindatatype, min, maxdatatype, max);
}

void dcSetValue::processAnimation(Animation *anim)
{
    if (datatype1 == FLOAT_TYPE)
    {
        float endval = *(float *)var;
        calculateValue(optype, FLOAT_TYPE, (void *)&endval, datatype2, val, mindatatype, min, maxdatatype, max);
        anim->addItem(var, *(float *)var, endval);
    }
}

void dcSetValue::calculateValue(int opspec, int vartype, void *varID, int valtype, void *valID, int mintype, void *minID, int maxtype, void *maxID)
{
    switch (vartype)
    {
        case FLOAT_TYPE:
            switch (opspec)
            {
                case PlusEquals:
                    *(float *)varID += getFloatVal(valtype, valID);
                    break;
                case MinusEquals:
                    *(float *)varID -= getFloatVal(valtype, valID);
                    break;
                default:
                    *(float *)varID = getFloatVal(valtype, valID);
            }
            if (minID)
            {
                float minval = getFloatVal(mintype, minID);
                if (*(float *)varID < minval) *(float *)varID = minval;
            }
            if (maxID)
            {
                float maxval = getFloatVal(maxtype, maxID);
                if (*(float *)varID > maxval) *(float *)varID = maxval;
            }
            break;
        case INTEGER_TYPE:
            switch (opspec)
            {
                case PlusEquals:
                    *(int *)varID += getIntegerVal(valtype, valID);
                    break;
                case MinusEquals:
                    *(int *)varID -= getIntegerVal(valtype, valID);
                    break;
                default:
                    *(int *)varID = getIntegerVal(valtype, valID);
            }
            if (minID)
            {
                int minval = getIntegerVal(mintype, minID);
                if (*(int *)varID < minval) *(int *)varID = minval;
            }
            if (maxID)
            {
                int maxval = getIntegerVal(maxtype, maxID);
                if (*(int *)varID > maxval) *(int *)varID = maxval;
            }
            break;
        case STRING_TYPE:
            switch (valtype)
            {
                case STRING_TYPE:
                    if (opspec == Equals)
                        strcpy((char *)varID, (char *)valID);
                    break;
            }
            break;
    }

    for (std::list<CommModule *>::iterator commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        (*commitem)->flagAsChanged(varID);
    }
}
