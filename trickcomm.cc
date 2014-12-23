#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trickcomm.hh"
#include "msg.hh"
#include "string_utils.hh"
#include "varlist.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0
#define ALLOCATION_CHUNK 10

TrickCommModule::TrickCommModule()
:
host(0x0),
port(0),
datarate(0x0),
disconnectaction(this->AppTerminate),
tvs(0x0)
{
#ifdef TRICKACTIVE
    this->tvs = new VariableServerComm;
#endif
}

TrickCommModule::~TrickCommModule()
{
    if (this->host) free(this->host);
    if (this->datarate) free(this->datarate);
#ifdef TRICKACTIVE
	int i;

	for (i=0; i<this->fromsim.count; i++) free(this->fromsim.data[i].trickvar);
	free(this->fromsim.data);

	for (i=0; i<this->tosim.count; i++) free(this->tosim.data[i].trickvar);
	free(this->tosim.data);

	delete this->tvs;
#endif
}

CommModule::CommStatus TrickCommModule::read(void)
{
#ifdef TRICKACTIVE
    if (!(this->active)) return this->Inactive;

	int i;
	int status = this->tvs->get();

    switch (status)
    {
        case VS_SUCCESS:
            for (i=0; i<this->fromsim.count; i++)
            {
                switch (this->fromsim.data[i].type)
                {
                    case VARLIST_FLOAT:
                        *(float *)this->fromsim.data[i].dcvalue = *(float *)this->fromsim.data[i].trickvalue;
                        break;
                    case VARLIST_INTEGER:
                        *(int *)this->fromsim.data[i].dcvalue = *(int *)this->fromsim.data[i].trickvalue;
                        break;
                    case VARLIST_STRING:
                        strcpy((char *)this->fromsim.data[i].dcvalue, (char *)this->fromsim.data[i].trickvalue);
                        break;
                }
                if (this->fromsim.data[i].type != VARLIST_UNKNOWN_TYPE && this->fromsim.data[i].init_only)
                {
                    if (this->tvs->remove_var(this->fromsim.data[i].trickvar) == VS_SUCCESS) this->fromsim.data[i].type = VARLIST_UNKNOWN_TYPE;
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
                this->active = 0;
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
        if (this->SecondsSinceLastConnectAttempt() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->activate();
            this->ResetLastConnectAttemptTime();
        }
    }

    if (this->active)
    {
        int i;

        for (i=0; i<this->tosim.count; i++)
        {
            switch (this->tosim.data[i].type)
            {
                case VARLIST_FLOAT:
                    if (this->tosim.data[i].forcewrite || *(float *)this->tosim.data[i].dcvalue != this->tosim.data[i].prevvalue.f)
                    {
                        this->tvs->put(this->tosim.data[i].trickvar, VS_FLOAT, this->tosim.data[i].dcvalue, this->tosim.data[i].units);
                        this->tosim.data[i].prevvalue.f = *(float *)this->tosim.data[i].dcvalue;
                        this->tosim.data[i].forcewrite = 0;
                    }
                    break;
                case VARLIST_INTEGER:
                    if (this->tosim.data[i].forcewrite || *(int *)this->tosim.data[i].dcvalue != this->tosim.data[i].prevvalue.i)
                    {
                        this->tvs->put(this->tosim.data[i].trickvar, VS_INTEGER, this->tosim.data[i].dcvalue, this->tosim.data[i].units);
                        this->tosim.data[i].prevvalue.i = *(int *)this->tosim.data[i].dcvalue;
                        this->tosim.data[i].forcewrite = 0;
                    }
                    break;
                case VARLIST_STRING:
                    if (this->tosim.data[i].forcewrite || strcmp((char *)this->tosim.data[i].dcvalue, this->tosim.data[i].prevvalue.str))
                    {
                        this->tvs->put(this->tosim.data[i].trickvar, VS_STRING, this->tosim.data[i].dcvalue, this->tosim.data[i].units);
                        strcpy(this->tosim.data[i].prevvalue.str, (char *)this->tosim.data[i].dcvalue);
                        this->tosim.data[i].forcewrite = 0;
                    }
                    break;
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
	int i;

	for (i=0; i<this->tosim.count; i++)
	{
        if (this->tosim.data[i].dcvalue == value) this->tosim.data[i].forcewrite = 1;
	}
#endif
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
    this->disconnectaction = this->AppReconnect;
}

void TrickCommModule::initializeParameterList(int bufID)
{
    TrickCommModule::io_parameter_list *io_map;

    switch (bufID)
    {
        case TrickCommModule::FromTrick:
            io_map = &(this->fromsim);
            break;
        case TrickCommModule::ToTrick:
            io_map = &(this->tosim);
            break;
        default:
            return;
    }
    io_map->count = 0;
    io_map->allocated_elements = 0;
    io_map->data = 0x0;
}

int TrickCommModule::addParameter(int bufID, const char *paramname, const char *trickvar, const char *units, const char *init_only)
{
#ifdef TRICKACTIVE
    TrickCommModule::io_parameter_list *io_map;
    void *valptr;

    switch (bufID)
    {
        case TrickCommModule::FromTrick:
            io_map = &(this->fromsim);
            break;
        case TrickCommModule::ToTrick:
            io_map = &(this->tosim);
            break;
        default:
            return this->Fail;
    }

    valptr = get_pointer(paramname);

    if (valptr)
    {
        if (io_map->count == io_map->allocated_elements)
        {
            io_map->allocated_elements += ALLOCATION_CHUNK;
            io_map->data = (TrickCommModule::io_parameter *)realloc(io_map->data, io_map->allocated_elements * sizeof(TrickCommModule::io_parameter));
            if (!(io_map->data)) return this->Fail;
        }
        io_map->data[io_map->count].trickvar = strdup(trickvar);

        if (units) io_map->data[io_map->count].units = strdup(units);
        else  io_map->data[io_map->count].units = strdup("--");

        io_map->data[io_map->count].type = get_datatype(paramname);
        io_map->data[io_map->count].dcvalue = valptr;
        io_map->data[io_map->count].prevvalue.i = 0;
        io_map->data[io_map->count].prevvalue.f = 0;
        bzero(io_map->data[io_map->count].prevvalue.str, STRING_DEFAULT_LENGTH);
        io_map->data[io_map->count].forcewrite = 0;
        io_map->data[io_map->count].init_only = BoolStrToInt(init_only, 0);
        io_map->count++;
    }

    return this->Success;
#else
    return this->Inactive;
#endif
}

void TrickCommModule::finishInitialization(void)
{
#ifdef TRICKACTIVE
	int i, type;

	for (i=0; i<this->fromsim.count; i++)
    {
        switch (this->fromsim.data[i].type)
        {
				case VARLIST_FLOAT:
					type = VS_FLOAT;
					break;
				case VARLIST_INTEGER:
					type = VS_INTEGER;
					break;
				case VARLIST_STRING:
					type = VS_STRING;
					break;
        }
        this->fromsim.data[i].trickvalue = this->tvs->add_var(this->fromsim.data[i].trickvar, this->fromsim.data[i].units, type, 1);
    }
#endif
}

void TrickCommModule::activate(void)
{
#ifdef TRICKACTIVE
    if (this->fromsim.count || this->tosim.count)
    {
	    if (this->tvs->activate(this->host, this->port, NULL, this->datarate) == VS_SUCCESS) this->active = 1;
	}
#endif
}
