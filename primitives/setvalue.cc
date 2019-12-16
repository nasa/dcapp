#include <cstring>
#include <list>
#include "nodes.hh"
#include "types.hh"
#include "varlist.hh"
#include "setvalue.hh"

extern appdata AppData;

enum { Equals, PlusEquals, MinusEquals };

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
    AppData.events.push_back(this);
}

void dcSetValue::updateData(void)
{
    calculateValue(optype, datatype1, var, datatype2, val, mindatatype, min, maxdatatype, max);
}

void dcSetValue::processAnimation(Animation *anim)
{
    if (datatype1 == DECIMAL_TYPE)
    {
        double endval = *(double *)var;
        calculateValue(optype, DECIMAL_TYPE, (void *)&endval, datatype2, val, mindatatype, min, maxdatatype, max);
        anim->addItem(var, *(double *)var, endval);
    }
}

void dcSetValue::calculateValue(int opspec, int vartype, void *varID, int valtype, void *valID, int mintype, void *minID, int maxtype, void *maxID)
{
    switch (vartype)
    {
        case DECIMAL_TYPE:
            switch (opspec)
            {
                case PlusEquals:
                    *(double *)varID += getDecimalValue(valtype, valID);
                    break;
                case MinusEquals:
                    *(double *)varID -= getDecimalValue(valtype, valID);
                    break;
                default:
                    *(double *)varID = getDecimalValue(valtype, valID);
            }
            if (minID)
            {
                double minval = getDecimalValue(mintype, minID);
                if (*(double *)varID < minval) *(double *)varID = minval;
            }
            if (maxID)
            {
                double maxval = getDecimalValue(maxtype, maxID);
                if (*(double *)varID > maxval) *(double *)varID = maxval;
            }
            break;
        case INTEGER_TYPE:
            switch (opspec)
            {
                case PlusEquals:
                    *(int *)varID += getIntegerValue(valtype, valID);
                    break;
                case MinusEquals:
                    *(int *)varID -= getIntegerValue(valtype, valID);
                    break;
                default:
                    *(int *)varID = getIntegerValue(valtype, valID);
            }
            if (minID)
            {
                int minval = getIntegerValue(mintype, minID);
                if (*(int *)varID < minval) *(int *)varID = minval;
            }
            if (maxID)
            {
                int maxval = getIntegerValue(maxtype, maxID);
                if (*(int *)varID > maxval) *(int *)varID = maxval;
            }
            break;
        case STRING_TYPE:
            if (opspec == Equals) *(std::string *)varID = getStringValue(valtype, valID);
            break;
    }

    for (const auto &commitem : AppData.commlist) commitem->flagAsChanged(varID);
}
