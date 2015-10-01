#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "edgecomm.hh"
#include "varlist.hh"
#include "timer.hh"

#define TIDY(a) if (a) { free(a); a=0x0; }

#define CONNECT_ATTEMPT_INTERVAL 2.0
#define ALLOCATION_CHUNK 10


EdgeCommModule::EdgeCommModule()
:
active(0),
cmd_group(0x0),
rcs(0x0)
{
    this->fromedge.count = 0;
    this->fromedge.allocated_elements = 0;
    this->fromedge.data = 0x0;
    this->toedge.count = 0;
    this->toedge.allocated_elements = 0;
    this->toedge.data = 0x0;

    this->last_connect_attempt = new Timer;
    this->edge_timer = new Timer;
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

    delete this->last_connect_attempt;
    delete this->edge_timer;
    delete this->rcs;
}

CommModule::CommStatus EdgeCommModule::read(void)
{
    if (!(this->active)) return this->Inactive;

    int i, ret;
    char *result = 0x0;
    char *cmd = 0x0, *strptr, *strval = 0x0;

    if (this->edge_timer->getSeconds() < this->update_rate) return this->None;

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

    this->edge_timer->restart();

    return this->Success;
}

CommModule::CommStatus EdgeCommModule::write(void)
{
    if (!(this->active))
    {
        if (this->last_connect_attempt->getSeconds() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->activate();
            this->last_connect_attempt->restart();
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
                    this->toedge.data[i].forcewrite = false;
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
                    this->toedge.data[i].forcewrite = false;
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
                    this->toedge.data[i].forcewrite = false;
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
        if (this->toedge.data[i].dcvalue == value) this->toedge.data[i].forcewrite = true;
    }
}

int EdgeCommModule::addParameter(int bufID, const char *paramname, const char *edgecmd)
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
            return this->Fail;
    }

    void *valptr = get_pointer(paramname);

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
        io_map->data[io_map->count].forcewrite = false;
        io_map->count++;
    }

    return this->Success;
}

int EdgeCommModule::finishInitialization(char *host, char *port, float spec_rate)
{
    if (this->rcs->initialize(host, port)) return this->Fail;
    this->update_rate = spec_rate;
    this->edge_timer->restart();
    return this->Success;
}

void EdgeCommModule::activate(void)
{
    int ret;
    char *cmd = 0x0;

    if (this->fromedge.count || this->toedge.count)
    {
        TIDY(this->cmd_group);
        if (this->rcs->send_doug_command("create_command_group", &(this->cmd_group), 0x0)) return;
        if (!this->cmd_group) return;
        if (!this->cmd_group[0]) return;

        for (int i=0; i<this->fromedge.count; i++)
        {
            if (asprintf(&cmd, "add_command_to_group %s \"%s\"", this->cmd_group, this->fromedge.data[i].edgecmd) == -1) return;
            ret = this->rcs->send_doug_command(cmd, 0x0, 0x0);
            TIDY(cmd);
            if (ret) return;
        }

        this->active = 1;
    }
}

int EdgeCommModule::isActive(void)
{
    return this->active;
}
