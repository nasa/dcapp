#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trickcomm.hh"
#include "trickio.hh"
#include "msg.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

TrickCommModule::TrickCommModule()
:
host(0x0),
port(0),
datarate(0x0),
disconnectaction(this->AppTerminate)
{
}

TrickCommModule::~TrickCommModule()
{
    if (this->host) free(this->host);
    if (this->datarate) free(this->datarate);
    trickio_term();
}

CommModule::CommStatus TrickCommModule::read(void)
{
    if (!(this->active)) return this->Inactive;

    int status = trickio_readsimdata();

    switch (status)
    {
        case TRICKIO_SUCCESS:
            return this->Success;
            break;
        case TRICKIO_PARTIAL_BUFFER:
            debug_msg("TrickComm received a partial buffer");
            return this->Fail;
            break;
        case TRICKIO_MANGLED_BUFFER:
            error_msg("TrickComm received a mangled data buffer");
            return this->Fail;
            break;
        case TRICKIO_INVALID_CONNECTION:
            user_msg("TrickComm connection terminated");
            if (this->disconnectaction == this->AppReconnect)
            {
                this->active = 0;
                return this->Fail;
            }
            else return this->Terminate;
            break;
        default:
            return this->None;
    }
}

CommModule::CommStatus TrickCommModule::write(void)
{
    if (!(this->active))
    {
        if (this->SecondsSinceLastConnectAttempt() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->activate();
            this->ResetLastConnectAttemptTime();
        }
    }

    if (this->active) trickio_writesimdata();

    return this->None;
}

void TrickCommModule::flagAsChanged(void *value)
{
    trickio_forcewrite(value);
}

void TrickCommModule::setHost(char *hostspec)
{
    if (hostspec) this->host = strdup(hostspec);
}

void TrickCommModule::setPort(int portspec)
{
    this->port = portspec;
}

void TrickCommModule::setDataRate(char *ratespec)
{
    if (ratespec) this->datarate = strdup(ratespec);
}

void TrickCommModule::setReconnectOnDisconnect(void)
{
    this->disconnectaction = TrickCommModule::AppReconnect;
}

void TrickCommModule::initializeParameterList(int bufID)
{
    trickio_initialize_parameter_list(bufID);
}

int TrickCommModule::addParameter(int bufID, const char *paramname, const char *trickvar, const char *units, const char *init_only)
{
    return trickio_add_parameter(bufID, paramname, trickvar, units, init_only);
}

void TrickCommModule::finishInitialization(void)
{
    trickio_finish_initialization();
}

void TrickCommModule::activate(void)
{
    if (trickio_activatecomm(this->host, this->port, this->datarate) == TRICKIO_SUCCESS) this->active = 1;
}

