#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "edgecomm.hh"
#include "varlist.hh"

#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))
#define TIDY(a) if (a) { free(a); a=0x0; }

#define CONNECT_ATTEMPT_INTERVAL 2.0
#define ALLOCATION_CHUNK 10


EdgeCommModule::EdgeCommModule()
:
active(0),
cmd_group(0x0),
rcs(0x0)
{
    this->rcs = new EdgeRcsComm;
}

EdgeCommModule::~EdgeCommModule()
{
	int i;

	for (i=0; i<this->fromedge.count; i++) TIDY(this->fromedge.data[i].edgecmd);
	TIDY(this->fromedge.data);
	for (i=0; i<this->toedge.count; i++) TIDY(this->toedge.data[i].edgecmd);
	TIDY(this->toedge.data);

    TIDY(this->cmd_group);

    delete this->rcs;
}

CommModule::CommStatus EdgeCommModule::read(void)
{
    if (!(this->active)) return this->Inactive;

	int i, ret;
	struct timeval now;
    char *result = 0x0;
    char *cmd = 0x0, *strptr, *strval = 0x0;

    gettimeofday(&now, 0x0);

    if (SecondsElapsed(this->edge_timer, now) < this->update_rate) return this->None;

    if (asprintf(&cmd, "execute_command_group %s", this->cmd_group) == -1)
    {
        this->active = 0;
        return this->Fail;
    }

    ret = this->rcs->send_doug_command(cmd, &result, 0x0);
    TIDY(cmd);
    if (ret)
    {
        this->active = 0;
        return this->Fail;
    }

    strval = (char *)calloc(1, strlen(result));
    if (!strval)
    {
        this->active = 0;
        return this->Fail;
    }

    for (i=0, strptr=result; i<this->fromedge.count; i++)
    {
        ret = sscanf(strptr, "%s", strval);
        if (!ret)
        {
            this->active = 0;
            return this->Fail;
        }

        switch (this->fromedge.data[i].type)
        {
            case VARLIST_FLOAT:
                *(float *)this->fromedge.data[i].dcvalue = strtof(strval, 0x0);
                break;
            case VARLIST_INTEGER:
                *(int *)this->fromedge.data[i].dcvalue = (int)strtol(strval, 0x0, 10);
                break;
            case VARLIST_STRING:
                strcpy((char *)this->fromedge.data[i].dcvalue, strval);
                break;
        }

        strptr += strlen(strval) + 1;
    }

    TIDY(strval);
    TIDY(result);

    this->edge_timer = now;

	return this->Success;
}

CommModule::CommStatus EdgeCommModule::write(void)
{
    if (!(this->active))
    {
        if (this->SecondsSinceLastConnectAttempt() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->activate();
            this->ResetLastConnectAttemptTime();
        }
    }

    if (!this->active) return this->None;

	int i, status = 0;
	char *cmd = 0x0;

	for (i=0; i<this->toedge.count; i++)
	{
		switch (this->toedge.data[i].type)
		{
			case VARLIST_FLOAT:
                if (this->toedge.data[i].forcewrite || *(float *)this->toedge.data[i].dcvalue != this->toedge.data[i].prevvalue.f)
                {
                    if (asprintf(&cmd, "%s %f", this->toedge.data[i].edgecmd, *(float *)(this->toedge.data[i].dcvalue)) == -1)
                    {
                        this->active = 0;
                        return this->Fail;
                    }
                    status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
                    this->toedge.data[i].prevvalue.f = *(float *)this->toedge.data[i].dcvalue;
                    this->toedge.data[i].forcewrite = 0;
		            TIDY(cmd);
                }
				break;
			case VARLIST_INTEGER:
                if (this->toedge.data[i].forcewrite || *(int *)this->toedge.data[i].dcvalue != this->toedge.data[i].prevvalue.i)
                {
                    if (asprintf(&cmd, "%s %d", this->toedge.data[i].edgecmd, *(int *)(this->toedge.data[i].dcvalue)) == -1)
                    {
                        this->active = 0;
                        return this->Fail;
                    }
                    status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
                    this->toedge.data[i].prevvalue.i = *(int *)this->toedge.data[i].dcvalue;
                    this->toedge.data[i].forcewrite = 0;
		            TIDY(cmd);
                }
				break;
			case VARLIST_STRING:
                if (this->toedge.data[i].forcewrite || strcmp((char *)this->toedge.data[i].dcvalue, this->toedge.data[i].prevvalue.str))
                {
                    if (asprintf(&cmd, "%s %s", this->toedge.data[i].edgecmd, (char *)(this->toedge.data[i].dcvalue)) == -1)
                    {
                        this->active = 0;
                        return this->Fail;
                    }
                    status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
                    strcpy(this->toedge.data[i].prevvalue.str, (char *)this->toedge.data[i].dcvalue);
                    this->toedge.data[i].forcewrite = 0;
		            TIDY(cmd);
                }
				break;
		}
	}

	if (status)
    {
        this->active = 0;
        return this->Fail;
    }
	else return this->Success;
}

void EdgeCommModule::flagAsChanged(void *value)
{
	int i;

	for (i=0; i<this->toedge.count; i++)
	{
        if (this->toedge.data[i].dcvalue == value) this->toedge.data[i].forcewrite = 1;
	}
}

void EdgeCommModule::initializeParameterList(int bufID)
{
    io_parameter_list *io_map;

    switch (bufID)
    {
        case EDGEIO_FROMEDGE:
            io_map = &(this->fromedge);
            break;
        case EDGEIO_TOEDGE:
            io_map = &(this->toedge);
            break;
        default:
            return;
    }
    io_map->count = 0;
    io_map->allocated_elements = 0;
    io_map->data = 0x0;
}

int EdgeCommModule::addParameter(int bufID, const char *paramname, const char *edgecmd)
{
    io_parameter_list *io_map;
    void *valptr;

    switch (bufID)
    {
        case EDGEIO_FROMEDGE:
            io_map = &(this->fromedge);
            break;
        case EDGEIO_TOEDGE:
            io_map = &(this->toedge);
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
            io_map->data = (io_parameter *)realloc(io_map->data, io_map->allocated_elements * sizeof(io_parameter));
            if (!io_map->data) return this->Fail;
        }
        io_map->data[io_map->count].edgecmd = strdup(edgecmd);
        io_map->data[io_map->count].type = get_datatype(paramname);
        io_map->data[io_map->count].dcvalue = valptr;
        io_map->data[io_map->count].prevvalue.i = 0;
        io_map->data[io_map->count].prevvalue.f = 0;
        bzero(io_map->data[io_map->count].prevvalue.str, STRING_DEFAULT_LENGTH);
        io_map->data[io_map->count].forcewrite = 0;
        io_map->count++;
    }
    
    return this->Success;
}

int EdgeCommModule::finishInitialization(char *host, char *port, float spec_rate)
{
    if (this->rcs->initialize(host, port)) return this->Fail;
    this->update_rate = spec_rate;
    gettimeofday(&(this->edge_timer), 0x0);
	return this->Success;
}

void EdgeCommModule::activate(void)
{
    int i, ret;
    char *cmd = 0x0;

    if (this->fromedge.count || this->toedge.count)
    {
        TIDY(this->cmd_group);
        if (this->rcs->send_doug_command("create_command_group", &(this->cmd_group), 0x0)) return;
        if (!this->cmd_group) return;
        if (!this->cmd_group[0]) return;

        for (i=0; i<this->fromedge.count; i++)
        {
            if (asprintf(&cmd, "add_command_to_group %s \"%s\"", this->cmd_group, this->fromedge.data[i].edgecmd) == -1) return;
            ret = this->rcs->send_doug_command(cmd, 0x0, 0x0);
            TIDY(cmd);
            if (ret) return;
        }

        this->active = 1;
    }
}
