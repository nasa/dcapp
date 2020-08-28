#include <string>
#include "app_data.hh"
#include "variables.hh"
#include "values.hh"
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

void dcSetValue::setOperator(const std::string &opspec)
{
    if (opspec.empty()) return;
    else if (opspec == "+=") optype = PlusEquals;
    else if (opspec == "-=") optype = MinusEquals;
}

void dcSetValue::setRange(const std::string &minspec, const std::string &maxspec)
{
    if (!minspec.empty()) min = getValueSSTR(minspec);
    if (!maxspec.empty()) max = getValueSSTR(maxspec);
}

void dcSetValue::draw(void)
{
    calculateValue(var);
    for (const auto &commitem : AppData.commlist) commitem->flagAsChanged(var);
}

void dcSetValue::handleEvent(void)
{
    AppData.events.push_back(this);
}

void dcSetValue::updateData(void)
{
    calculateValue(var);
    for (const auto &commitem : AppData.commlist) commitem->flagAsChanged(var);
}

void dcSetValue::processAnimation(Animation *anim)
{
    Variable endval;
    endval.setType(DECIMAL_TYPE);
    endval.setToDecimal(var->getDecimal());
    calculateValue(&endval);
    anim->addItem(var, endval.getDecimal());
}

void dcSetValue::calculateValue(Variable *varID)
{
    switch (optype)
    {
        case PlusEquals:
            varID->incrementByValue(*val);
            break;
        case MinusEquals:
            varID->decrementByValue(*val);
            break;
        default:
            varID->setToValue(*val);
    }
    if (min) varID->applyMinimumByValue(*min);
    if (max) varID->applyMaximumByValue(*max);
}
