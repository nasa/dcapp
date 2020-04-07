#ifdef TRICKACTIVE

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "basicutils/msg.hh"
#include "basicutils/timer.hh"
#include "trickcomm.hh"
#include "string_utils.hh"
#include "types.hh"
#include "varlist.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

TrickCommModule::TrickCommModule()
:
active(false),
host(0x0),
port(0),
datarate(0x0),
disconnectaction(this->AppTerminate)
{
    this->last_connect_attempt = new Timer;
    this->tvs = new VariableServerComm;
}

TrickCommModule::~TrickCommModule()
{
    if (this->host) free(this->host);
    if (this->datarate) free(this->datarate);
    fromSim.clear();
    toSim.clear();

    delete this->last_connect_attempt;
    delete this->tvs;
}

CommModule::CommStatus TrickCommModule::read(void)
{
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
                    case DECIMAL_TYPE:
                        *(double *)myitem->dcvalue = myitem->trickvalue->decval;
                        break;
                    case INTEGER_TYPE:
                        *(int *)myitem->dcvalue = myitem->trickvalue->intval;
                        break;
                    case STRING_TYPE:
                        *(std::string *)myitem->dcvalue = myitem->trickvalue->strval;
                        break;
                }
                if (myitem->type != UNDEFINED_TYPE && myitem->init_only)
                {
                    this->tvs->remove_var(myitem->trickvar);
                    this->fromSim.erase(myitem);
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
}

CommModule::CommStatus TrickCommModule::write(void)
{
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
                    case DECIMAL_TYPE:
                        if (*(double *)myitem->dcvalue)
                        {
                            this->tvs->putMethod(myitem->trickvar);
                            *(double *)myitem->dcvalue = 0;
                        }
                        break;
                    case INTEGER_TYPE:
                        if (*(int *)myitem->dcvalue)
                        {
                            this->tvs->putMethod(myitem->trickvar);
                            *(int *)myitem->dcvalue = 0;
                        }
                        break;
                    case STRING_TYPE:
                        if (StringToBoolean(*(std::string *)myitem->dcvalue))
                        {
                            this->tvs->putMethod(myitem->trickvar);
                            *(std::string *)myitem->dcvalue = "false";
                        }
                        break;
                }
            }
            else
            {
                switch (myitem->type)
                {
                    case DECIMAL_TYPE:
                        if (myitem->forcewrite || *(double *)myitem->dcvalue != myitem->prevvalue.decval)
                        {
                            this->tvs->putValue(myitem->trickvar, *(double *)myitem->dcvalue, myitem->units);
                            myitem->prevvalue.decval = *(double *)myitem->dcvalue;
                            myitem->forcewrite = false;
                        }
                        break;
                    case INTEGER_TYPE:
                        if (myitem->forcewrite || *(int *)myitem->dcvalue != myitem->prevvalue.intval)
                        {
                            this->tvs->putValue(myitem->trickvar, *(int *)myitem->dcvalue, myitem->units);
                            myitem->prevvalue.intval = *(int *)myitem->dcvalue;
                            myitem->forcewrite = false;
                        }
                        break;
                    case STRING_TYPE:
                        if (myitem->forcewrite || *(std::string *)myitem->dcvalue != myitem->prevvalue.strval)
                        {
                            this->tvs->putValue(myitem->trickvar, *(std::string *)myitem->dcvalue, myitem->units);
                            myitem->prevvalue.strval = *(std::string *)myitem->dcvalue;
                            myitem->forcewrite = false;
                        }
                        break;
                }
            }
        }
    }

    return this->None;
}

void TrickCommModule::flagAsChanged(void *value)
{
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toSim.begin(); myitem != this->toSim.end(); myitem++)
    {
        if (myitem->dcvalue == value) myitem->forcewrite = true;
    }
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
        else  myparam.units = nullptr;

        myparam.type = get_datatype(paramname);
        myparam.dcvalue = valptr;
        myparam.forcewrite = false;
        myparam.init_only = StringToBoolean(init_only);
        myparam.method = method;
        io_map->push_back(myparam);
    }

    return this->Success;
}

void TrickCommModule::finishInitialization(void)
{
    int type = 0;
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->fromSim.begin(); myitem != this->fromSim.end(); myitem++)
    {
        switch (myitem->type)
        {
                case DECIMAL_TYPE: type = VS_DECIMAL; break;
                case INTEGER_TYPE: type = VS_INTEGER; break;
                case STRING_TYPE:  type = VS_STRING;  break;
        }
        if (type) myitem->trickvalue = this->tvs->add_var(myitem->trickvar, myitem->units, type);
    }
}

void TrickCommModule::activate(void)
{
    if (!(this->fromSim.empty()) || !(this->toSim.empty()))
    {
        if (this->tvs->activate(this->host, this->port, 0x0, this->datarate) == VS_SUCCESS) this->active = true;
    }
}

bool TrickCommModule::isActive(void)
{
    return this->active;
}

#else

#include "basicutils/msg.hh"
#include "trickcomm.hh"

TrickCommModule::TrickCommModule()
{
    warning_msg("Trick communication requested, but Trick doesn't seem to be properly installed...");
}

int TrickCommModule::addParameter(int, const char *, const char *, const char *, const char *, int)
{
    return this->Inactive;
}

#endif
