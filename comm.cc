#include "variables.hh"
#include "comm.hh"

CommModule::CommModule() : active(false), activeID(0x0)
{
}

CommModule::~CommModule()
{
}

void CommModule::setConnectedVariable(const char *spec)
{
    if (spec) this->activeID = getVariable(spec);
}

CommModule::CommStatus CommModule::read(void)
{
    return this->None;
}

CommModule::CommStatus CommModule::write(void)
{
    return this->None;
}

void CommModule::flagAsChanged(Variable *)
{
    return;
}

void CommModule::updateConnectedVariable(void)
{
    if (this->activeID) this->activeID->setToBoolean(this->active);
}
