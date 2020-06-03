#ifdef TRICKACTIVE

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "variables.hh"
#include "trickcomm.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

TrickCommModule::TrickCommModule() : host(0x0), port(0), datarate(0x0), disconnectaction(this->AppTerminate)
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
                myitem->currvalue->setToValue(*(myitem->trickvalue));
                if (myitem->init_only)
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
                if (myitem->currvalue->getBoolean())
                {
                    this->tvs->putMethod(myitem->trickvar);
                    myitem->currvalue->setToBoolean(false);
                }
            }
            else
            {
                if (myitem->forcewrite || *(myitem->currvalue) != myitem->prevvalue)
                {
                    this->tvs->putValue(myitem->trickvar, *(myitem->currvalue), myitem->units);
                    myitem->prevvalue.setToValue(*(myitem->currvalue));
                    myitem->forcewrite = false;
                }
            }
        }
    }

    return this->None;
}

void TrickCommModule::flagAsChanged(Variable *value)
{
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toSim.begin(); myitem != this->toSim.end(); myitem++)
    {
        if (myitem->currvalue == value) myitem->forcewrite = true;
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

int TrickCommModule::addParameter(int bufID, const char *paramname, const char *trickvar, const char *units, const char *init_only, bool method)
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

    Variable *myvalue = getVariable(paramname);

    if (myvalue)
    {
        io_parameter myparam;
        myparam.trickvar = strdup(trickvar);

        if (units) myparam.units = strdup(units);
        else  myparam.units = nullptr;

        myparam.currvalue = myvalue;
        myparam.prevvalue.setAttributes(*myvalue);
        myparam.forcewrite = false;
        myparam.init_only = StringToBoolean(init_only);
        myparam.method = method;
        io_map->push_back(myparam);

        return this->Success;
    }
    else return this->Fail;
}

void TrickCommModule::finishInitialization(void)
{
    for (std::list<io_parameter>::iterator myitem = this->fromSim.begin(); myitem != this->fromSim.end(); myitem++)
    {
        myitem->trickvalue = this->tvs->add_var(myitem->trickvar, myitem->units, *(myitem->currvalue));
    }
}

void TrickCommModule::activate(void)
{
    if (!(this->fromSim.empty()) || !(this->toSim.empty()))
    {
        if (this->tvs->activate(this->host, this->port, 0x0, this->datarate) == VS_SUCCESS) this->active = true;
    }
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
