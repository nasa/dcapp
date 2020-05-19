#include <cstring>
#include <list>
#include "nodes.hh"
#include "valuedata.hh"
#include "varlist.hh"
#include "setvalue.hh"

extern appdata AppData;

enum { Equals, PlusEquals, MinusEquals };

dcSetValue::dcSetValue(dcParent *myparent, const char *invar, const char *inval) : optype(Equals), min(0x0), max(0x0)
{
    if (!invar || !inval) return;

    var = getVariable(invar);

    // don't parent this object if var isn't properly defined
    if (!var) return;

    val = getValue(inval);

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
    if (minspec) min = getValue(minspec);
    if (maxspec) max = getValue(maxspec);
}

void dcSetValue::draw(void)
{
    calculateValue(optype, var, val, min, max);
}

void dcSetValue::handleEvent(void)
{
    AppData.events.push_back(this);
}

void dcSetValue::updateData(void)
{
    calculateValue(optype, var, val, min, max);
}

void dcSetValue::processAnimation(Animation *anim)
{
    Variable endval;
    endval.setType(DECIMAL_TYPE);
    endval.setToDecimal(var->getDecimal());
    calculateValue(optype, &endval, val, min, max);
    anim->addItem(var->getPointer(), var->getDecimal(), endval.getDecimal());
}

void dcSetValue::calculateValue(int opspec, Variable *varID, Value *valID, Value *minID, Value *maxID)
{
    switch (opspec)
    {
        case PlusEquals:
            varID->incrementByValue(*valID);
            break;
        case MinusEquals:
            varID->decrementByValue(*valID);
            break;
        default:
            varID->setToValue(*valID);
    }
    if (minID) varID->applyMinimumByValue(*minID);
    if (maxID) varID->applyMaximumByValue(*maxID);

    for (const auto &commitem : AppData.commlist) commitem->flagAsChanged(varID);
}
