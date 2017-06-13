#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "edgecomm.hh"
#include "varlist.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0


EdgeCommModule::EdgeCommModule()
:
active(false),
cmd_group(0x0),
rcs(0x0)
{
    this->last_connect_attempt = new Timer;
    this->edge_timer = new Timer;
    this->rcs = new EdgeRcsComm;
}

EdgeCommModule::~EdgeCommModule()
{
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->fromEdge.begin(); myitem != this->fromEdge.end(); myitem++) TIDY(myitem->edgecmd);
    fromEdge.clear();
    for (myitem = this->toEdge.begin(); myitem != this->toEdge.end(); myitem++) TIDY(myitem->edgecmd);
    toEdge.clear();

    TIDY(this->cmd_group);

    delete this->last_connect_attempt;
    delete this->edge_timer;
    delete this->rcs;
}

CommModule::CommStatus EdgeCommModule::read(void)
{
    if (!(this->active)) return this->Inactive;

    int ret;
    char *result = 0x0;
    char *cmd = 0x0, *strptr, *strval = 0x0;

    if (this->edge_timer->getSeconds() < this->update_rate) return this->None;

    if (asprintf(&cmd, "execute_command_group %s", this->cmd_group) == -1)
    {
        this->active = false;
        return this->Fail;
    }

    ret = this->rcs->send_doug_command(cmd, &result, 0x0);
    TIDY(cmd);
    if (ret)
    {
        this->active = false;
        return this->Fail;
    }

    strval = (char *)calloc(1, strlen(result));
    if (!strval)
    {
        this->active = false;
        return this->Fail;
    }

    std::list<io_parameter>::iterator myitem;
    strptr = result;

    for (myitem = this->fromEdge.begin(); myitem != this->fromEdge.end(); myitem++)
    {
        ret = sscanf(strptr, "%s", strval);
        if (!ret)
        {
            this->active = false;
            return this->Fail;
        }

        switch (myitem->type)
        {
            case VARLIST_FLOAT:
                *(float *)myitem->dcvalue = strtof(strval, 0x0);
                break;
            case VARLIST_INTEGER:
                *(int *)myitem->dcvalue = (int)strtol(strval, 0x0, 10);
                break;
            case VARLIST_STRING:
                strcpy((char *)myitem->dcvalue, strval);
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

    if (!(this->active)) return this->None;

    int status = 0;
    char *cmd = 0x0;

    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toEdge.begin(); myitem != this->toEdge.end(); myitem++)
    {
        switch (myitem->type)
        {
            case VARLIST_FLOAT:
                if (myitem->forcewrite || *(float *)myitem->dcvalue != myitem->prevvalue.f)
                {
                    if (asprintf(&cmd, "%s %f", myitem->edgecmd, *(float *)(myitem->dcvalue)) == -1)
                    {
                        this->active = false;
                        return this->Fail;
                    }
                    status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
                    myitem->prevvalue.f = *(float *)myitem->dcvalue;
                    myitem->forcewrite = false;
                    TIDY(cmd);
                }
                break;
            case VARLIST_INTEGER:
                if (myitem->forcewrite || *(int *)myitem->dcvalue != myitem->prevvalue.i)
                {
                    if (asprintf(&cmd, "%s %d", myitem->edgecmd, *(int *)(myitem->dcvalue)) == -1)
                    {
                        this->active = false;
                        return this->Fail;
                    }
                    status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
                    myitem->prevvalue.i = *(int *)myitem->dcvalue;
                    myitem->forcewrite = false;
                    TIDY(cmd);
                }
                break;
            case VARLIST_STRING:
                if (myitem->forcewrite || strcmp((char *)myitem->dcvalue, myitem->prevvalue.str))
                {
                    if (asprintf(&cmd, "%s %s", myitem->edgecmd, (char *)(myitem->dcvalue)) == -1)
                    {
                        this->active = false;
                        return this->Fail;
                    }
                    status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
                    strcpy(myitem->prevvalue.str, (char *)myitem->dcvalue);
                    myitem->forcewrite = false;
                    TIDY(cmd);
                }
                break;
        }
    }

    if (status)
    {
        this->active = false;
        return this->Fail;
    }
    else return this->Success;
}

void EdgeCommModule::flagAsChanged(void *value)
{
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toEdge.begin(); myitem != this->toEdge.end(); myitem++)
    {
        if (myitem->dcvalue == value) myitem->forcewrite = true;
    }
}

int EdgeCommModule::addParameter(int bufID, const char *paramname, const char *edgecmd)
{
    std::list<io_parameter> *io_map;

    switch (bufID)
    {
        case EDGEIO_FROMEDGE:
            io_map = &(this->fromEdge);
            break;
        case EDGEIO_TOEDGE:
            io_map = &(this->toEdge);
            break;
        default:
            return this->Fail;
    }

    void *valptr = get_pointer(paramname);

    if (valptr)
    {
        io_parameter myparam;
        myparam.edgecmd = strdup(edgecmd);
        myparam.type = get_datatype(paramname);
        myparam.dcvalue = valptr;
        myparam.prevvalue.i = 0;
        myparam.prevvalue.f = 0;
        bzero(myparam.prevvalue.str, STRING_DEFAULT_LENGTH);
        myparam.forcewrite = false;
        io_map->push_back(myparam);
    }

    return this->Success;
}

int EdgeCommModule::finishInitialization(const char *host, const char *port, float spec_rate)
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

    if (!(this->fromEdge.empty()) || !(this->toEdge.empty()))
    {
        TIDY(this->cmd_group);
        if (this->rcs->send_doug_command("create_command_group", &(this->cmd_group), 0x0)) return;
        if (!(this->cmd_group)) return;
        if (!(this->cmd_group[0])) return;

        std::list<io_parameter>::iterator myitem;

        for (myitem = this->fromEdge.begin(); myitem != this->fromEdge.end(); myitem++)
        {
            if (asprintf(&cmd, "add_command_to_group %s \"%s\"", this->cmd_group, myitem->edgecmd) == -1) return;
            ret = this->rcs->send_doug_command(cmd, 0x0, 0x0);
            TIDY(cmd);
            if (ret) return;
        }

        this->active = true;
    }
}

bool EdgeCommModule::isActive(void)
{
    return this->active;
}
