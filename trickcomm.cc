#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "utils/msg.hh"
#include "utils/timer.hh"
#include "trickcomm.hh"
#include "string_utils.hh"
#include "varlist.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

TrickCommModule::TrickCommModule()
:
active(false),
#ifdef TRICKACTIVE
tvs(0x0),
#endif
host(0x0),
port(0),
datarate(0x0),
disconnectaction(this->AppTerminate)
{
#ifdef TRICKACTIVE
    this->last_connect_attempt = new Timer;
    this->tvs = new VariableServerComm;
#endif
}

TrickCommModule::~TrickCommModule()
{
    if (this->host) free(this->host);
    if (this->datarate) free(this->datarate);
#ifdef TRICKACTIVE
    fromSim.clear();
    toSim.clear();

    delete this->last_connect_attempt;
    delete this->tvs;
#endif
}

CommModule::CommStatus TrickCommModule::read(void)
{
#ifdef TRICKACTIVE
    if (!(this->active)) return this->Inactive;

    int status = this->tvs->get();
    std::list<io_parameter>::iterator myitem;

    switch (status)
    {
        case VS_SUCCESS:
            for (myitem = this->fromSim.begin(); myitem != this->fromSim.end(); myitem++)
            {
                switch (myitem->type)
                {
                    case VARLIST_FLOAT:
                        *(float *)myitem->dcvalue = *(float *)myitem->trickvalue;
                        break;
                    case VARLIST_INTEGER:
                        *(int *)myitem->dcvalue = *(int *)myitem->trickvalue;
                        break;
                    case VARLIST_STRING:
                        strcpy((char *)myitem->dcvalue, (char *)myitem->trickvalue);
                        break;
                }
                if (myitem->type != VARLIST_UNKNOWN_TYPE && myitem->init_only)
                {
                    if (this->tvs->remove_var(myitem->trickvar) == VS_SUCCESS) this->fromSim.erase(myitem);
                }
            }
            return this->Success;
        case VS_PARTIAL_BUFFER:
            debug_msg("TrickComm received a partial buffer");
            return this->Fail;
        case VS_MANGLED_BUFFER:
            error_msg("TrickComm received a mangled data buffer");
            return this->Fail;
        case VS_INVALID_CONNECTION:
            user_msg("TrickComm connection terminated");
            if (this->disconnectaction == this->AppReconnect)
            {
                this->active = false;
                return this->Fail;
            }
            else return this->Terminate;
        default:
            return this->None;
    }
#else
    return this->Inactive;
#endif
}

CommModule::CommStatus TrickCommModule::write(void)
{
#ifdef TRICKACTIVE
    if (!(this->active))
    {
        if (this->last_connect_attempt->getSeconds() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->activate();
            this->last_connect_attempt->restart();
        }
    }

    if (this->active)
    {
        std::list<io_parameter>::iterator myitem;

        for (myitem = this->toSim.begin(); myitem != this->toSim.end(); myitem++)
        {
            if (myitem->method)
            {
                switch (myitem->type)
                {
                    case VARLIST_FLOAT:
                        if (*(float *)myitem->dcvalue)
                        {
                            this->tvs->put(myitem->trickvar, VS_METHOD, 0x0, 0x0);
                            *(float *)myitem->dcvalue = 0;
                        }
                        break;
                    case VARLIST_INTEGER:
                        if (*(int *)myitem->dcvalue)
                        {
                            this->tvs->put(myitem->trickvar, VS_METHOD, 0x0, 0x0);
                            *(int *)myitem->dcvalue = 0;
                        }
                        break;
                    case VARLIST_STRING:
                        if (StrToBool((char *)myitem->dcvalue, false))
                        {
                            this->tvs->put(myitem->trickvar, VS_METHOD, 0x0, 0x0);
                            strcpy((char *)myitem->dcvalue, "false");
                        }
                        break;
                }
            }
            else
            {
                switch (myitem->type)
                {
                    case VARLIST_FLOAT:
                        if (myitem->forcewrite || *(float *)myitem->dcvalue != myitem->prevvalue.f)
                        {
                            this->tvs->put(myitem->trickvar, VS_FLOAT, myitem->dcvalue, myitem->units);
                            myitem->prevvalue.f = *(float *)myitem->dcvalue;
                            myitem->forcewrite = false;
                        }
                        break;
                    case VARLIST_INTEGER:
                        if (myitem->forcewrite || *(int *)myitem->dcvalue != myitem->prevvalue.i)
                        {
                            this->tvs->put(myitem->trickvar, VS_INTEGER, myitem->dcvalue, myitem->units);
                            myitem->prevvalue.i = *(int *)myitem->dcvalue;
                            myitem->forcewrite = false;
                        }
                        break;
                    case VARLIST_STRING:
                        if (myitem->forcewrite || strcmp((char *)myitem->dcvalue, myitem->prevvalue.str))
                        {
                            this->tvs->put(myitem->trickvar, VS_STRING, myitem->dcvalue, myitem->units);
                            strcpy(myitem->prevvalue.str, (char *)myitem->dcvalue);
                            myitem->forcewrite = false;
                        }
                        break;
                }
            }
        }
    }

    return this->None;
#else
    return this->Inactive;
#endif
}

void TrickCommModule::flagAsChanged(void *value)
{
#ifdef TRICKACTIVE
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toSim.begin(); myitem != this->toSim.end(); myitem++)
    {
        if (myitem->dcvalue == value) myitem->forcewrite = true;
    }
#endif
}

void TrickCommModule::setHost(const char *hostspec)
{
    if (hostspec) this->host = strdup(hostspec);
}

void TrickCommModule::setPort(int portspec)
{
    this->port = portspec;
}

void TrickCommModule::setDataRate(const char *ratespec)
{
    if (ratespec) this->datarate = strdup(ratespec);
}

void TrickCommModule::setReconnectOnDisconnect(void)
{
    this->disconnectaction = this->AppReconnect;
}

int TrickCommModule::addParameter(int bufID, const char *paramname, const char *trickvar, const char *units, const char *init_only, int method)
{
#ifdef TRICKACTIVE
    std::list<io_parameter> *io_map;

    switch (bufID)
    {
        case TrickCommModule::FromTrick:
            io_map = &(this->fromSim);
            break;
        case TrickCommModule::ToTrick:
            io_map = &(this->toSim);
            break;
        default:
            return this->Fail;
    }

    void *valptr = get_pointer(paramname);

    if (valptr)
    {
        io_parameter myparam;
        myparam.trickvar = strdup(trickvar);

        if (units) myparam.units = strdup(units);
        else  myparam.units = strdup("--");

        myparam.type = get_datatype(paramname);
        myparam.dcvalue = valptr;
        myparam.prevvalue.i = 0;
        myparam.prevvalue.f = 0;
        bzero(myparam.prevvalue.str, STRING_DEFAULT_LENGTH);
        myparam.forcewrite = false;
        myparam.init_only = StrToBool(init_only, false);
        myparam.method = method;
        io_map->push_back(myparam);
    }

    return this->Success;
#else
    return this->Inactive;
#endif
}

void TrickCommModule::finishInitialization(void)
{
#ifdef TRICKACTIVE
    int type, nelem;
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->fromSim.begin(); myitem != this->fromSim.end(); myitem++)
    {
        switch (myitem->type)
        {
                case VARLIST_FLOAT:
                    type = VS_FLOAT;
                    nelem = 1;
                    break;
                case VARLIST_INTEGER:
                    type = VS_INTEGER;
                    nelem = 1;
                    break;
                case VARLIST_STRING:
                    type = VS_STRING;
                    nelem = STRING_DEFAULT_LENGTH;
                    break;
                default:
                    type = 0;
                    break;
        }
        if (type) myitem->trickvalue = this->tvs->add_var(myitem->trickvar, myitem->units, type, nelem);
    }

#endif
}

void TrickCommModule::activate(void)
{
#ifdef TRICKACTIVE
    if (!(this->fromSim.empty()) || !(this->toSim.empty()))
    {
        if (this->tvs->activate(this->host, this->port, 0x0, this->datarate) == VS_SUCCESS) this->active = true;
    }
#endif
}

bool TrickCommModule::isActive(void)
{
    return this->active;
}
