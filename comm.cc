#include "comm.hh"

CommModule::CommModule()
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
