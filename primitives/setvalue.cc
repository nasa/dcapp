#include "xml_data.hh"
#include "app_data.hh"
#include "variables.hh"
#include "values.hh"
#include "setvalue.hh"

extern appdata AppData;

enum { Equals, PlusEquals, MinusEquals, MultiplyEquals, DivideEquals };

dcSetValue::dcSetValue(dcParent *myparent, const xmldata &invar, const xmldata &inval) : optype(Equals), min(0x0), max(0x0)
{
    if (!invar.defined() || !inval.defined()) return;

    var = getVariable(invar);

    // don't parent this object if var isn't properly defined
    if (!var) return;

    val = getValue(inval);

    // this object doesn't require a parent since it can be used during preprocessing
    if (myparent) myparent->addChild(this);
}

void dcSetValue::setOperator(const xmldata &opspec)
{
    if (opspec.empty()) return;
    else if (opspec == "+=") optype = PlusEquals;
    else if (opspec == "-=") optype = MinusEquals;
    else if (opspec == "*=") optype = MultiplyEquals;
    else if (opspec == "/=") optype = DivideEquals;
}

void dcSetValue::setRange(const xmldata &minspec, const xmldata &maxspec)
{
    if (!minspec.empty()) min = getValue(minspec);
    if (!maxspec.empty()) max = getValue(maxspec);
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
        case MultiplyEquals:
            varID->multiplyByValue(*val);
            break;
        case DivideEquals:
            varID->divideByValue(*val);
            break;
        default:
            varID->setToValue(*val);
    }
    if (min) varID->applyMinimumByValue(*min);
    if (max) varID->applyMaximumByValue(*max);
}
