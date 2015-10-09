#include "comm.hh"

CommModule::CommModule()
:
activeID(0x0)
{
}

CommModule::~CommModule()
{
}

CommModule::CommStatus CommModule::read(void)
{
    return this->None;
}

CommModule::CommStatus CommModule::write(void)
{
    return this->None;
}

void CommModule::flagAsChanged(void *)
{
    return;
}

bool CommModule::isActive(void)
{
    return false;
}
