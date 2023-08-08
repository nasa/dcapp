#include "basicutils/msg.hh"
#include "trickcomm.hh"

#ifdef TRICKACTIVE

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "variables.hh"
#include "trickcomm.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

TrickCommModule::TrickCommModule() : port(0), disconnectaction(this->AppTerminate), io_map(0x0)
{
    this->last_connect_attempt = new Timer;
    this->tvs = new VariableServerComm;
}

TrickCommModule::~TrickCommModule()
{
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
                    std::string paramString = "(";
                    for (auto & param: myitem->params) {
                        paramString += param->getString();
                        paramString += ",";
                    }
                    if ( myitem->params.size() > 0) {
                        paramString.pop_back();
                    }
                    paramString += ")";
                    this->tvs->putMethod(myitem->trickvar, paramString);
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

void TrickCommModule::setHost(const std::string &hostspec)
{
    this->host = hostspec;
}

void TrickCommModule::setPort(const std::string &portspec)
{
    this->port = StringToInteger(portspec);
}

void TrickCommModule::setDataRate(const std::string &ratespec)
{
    this->datarate = ratespec;
}

void TrickCommModule::setReconnectOnDisconnect(void)
{
    this->disconnectaction = this->AppReconnect;
}

int TrickCommModule::addParameter(const std::string &paramname, const std::string &trickvar, const std::string &units, const std::string &init_only, std::vector<std::string> paramList, bool method)
{
    if (paramname.empty() || trickvar.empty() || !io_map) return this->Fail;
    if (method && io_map != &(this->toSim)) return this->Fail;

    Variable *myvalue = getVariable(paramname);

    if (myvalue)
    {
        io_parameter myparam;

        myparam.trickvar = trickvar;
        myparam.units = units;
        myparam.currvalue = myvalue;
        myparam.prevvalue.setAttributes(*myvalue);
        myparam.forcewrite = false;
        myparam.init_only = StringToBoolean(init_only);
        myparam.method = method;

        for (auto & param : paramList) {
            myparam.params.push_back(getValue(param));
        }

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
        if (this->tvs->activate(this->host, this->port, this->datarate) == VS_SUCCESS) this->active = true;
    }
}

#else

TrickCommModule::TrickCommModule()
{
    warning_msg("Trick communication requested, but Trick doesn't seem to be properly installed...");
}

#endif
