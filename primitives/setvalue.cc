#include <cstring>
#include <list>
#include "nodes.hh"
#include "types.hh"
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
    if (var->getType() == UNDEFINED_TYPE) return;

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
    if (var->getType() == DECIMAL_TYPE)
    {
        Variable *endval = new Variable;
        endval->setType(DECIMAL_TYPE);
        endval->setToDecimal(var->getDecimal());
        calculateValue(optype, endval, val, min, max);
        anim->addItem(var->getPointer(), var->getDecimal(), endval->getDecimal());
    }
}

void dcSetValue::calculateValue(int opspec, Variable *varID, Value *valID, Value *minID, Value *maxID)
{
    switch (varID->getType())
    {
        case DECIMAL_TYPE:
            switch (opspec)
            {
                case PlusEquals:
                    *(double *)varID->getPointer() += valID->getDecimal();
                    break;
                case MinusEquals:
                    *(double *)varID->getPointer() -= valID->getDecimal();
                    break;
                default:
                    *(double *)varID->getPointer() = valID->getDecimal();
            }
            if (minID)
            {
                double minval = minID->getDecimal();
                if (*(double *)varID->getPointer() < minval) *(double *)varID->getPointer() = minval;
            }
            if (maxID)
            {
                double maxval = maxID->getDecimal();
                if (*(double *)varID->getPointer() > maxval) *(double *)varID->getPointer() = maxval;
            }
            break;
        case INTEGER_TYPE:
            switch (opspec)
            {
                case PlusEquals:
                    *(int *)varID->getPointer() += valID->getInteger();
                    break;
                case MinusEquals:
                    *(int *)varID->getPointer() -= valID->getInteger();
                    break;
                default:
                    *(int *)varID->getPointer() = valID->getInteger();
            }
            if (minID)
            {
                int minval = minID->getInteger();
                if (*(int *)varID->getPointer() < minval) *(int *)varID->getPointer() = minval;
            }
            if (maxID)
            {
                int maxval = maxID->getInteger();
                if (*(int *)varID->getPointer() > maxval) *(int *)varID->getPointer() = maxval;
            }
            break;
        case STRING_TYPE:
            if (opspec == Equals) *(std::string *)varID->getPointer() = valID->getString();
            break;
    }

    for (const auto &commitem : AppData.commlist) commitem->flagAsChanged(varID);
}
