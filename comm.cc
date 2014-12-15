#include "comm.hh"

#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))

CommModule::CommModule()
:
active(0)
{
    this->ResetLastConnectAttemptTime();
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

void CommModule::setForceWrite(void *)
{
    return;
}

void CommModule::ResetLastConnectAttemptTime(void)
{
    gettimeofday(&(this->last_connect_attempt), NULL);
}

float CommModule::SecondsSinceLastConnectAttempt(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return SecondsElapsed(this->last_connect_attempt, now);
}
