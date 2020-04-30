#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "varlist.hh"
#include "valuedata.hh"
#include "edgecomm.hh"

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

    fromEdge.clear();
    toEdge.clear();

    TIDY(this->cmd_group);

    delete this->last_connect_attempt;
    delete this->edge_timer;
    delete this->rcs;
}

CommModule::CommStatus EdgeCommModule::read(void)
{
    if (!(this->active)) return this->Inactive;
    if (this->edge_timer->getSeconds() < this->update_rate) return this->None;

    int ret;
    char *result = 0x0;
    char *strptr, *substr = 0x0;
    std::string cmd = "execute_command_group ";

    cmd += this->cmd_group;

    ret = this->rcs->send_doug_command(cmd, &result, 0x0);
    if (ret)
    {
        this->active = false;
        return this->Fail;
    }

    substr = (char *)calloc(1, strlen(result));
    if (!substr)
    {
        this->active = false;
        return this->Fail;
    }

    strptr = result;

    std::list<io_parameter>::iterator myitem;
    for (myitem = this->fromEdge.begin(); myitem != this->fromEdge.end(); myitem++)
    {
        ret = sscanf(strptr, "%s", substr);
        if (!ret)
        {
            this->active = false;
            return this->Fail;
        }

// maybe verify that type is legit here?
        myitem->currvalue->setToCharstr(substr);

        strptr += strlen(substr) + 1;
    }

    TIDY(substr);
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
    std::string cmd;

    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toEdge.begin(); myitem != this->toEdge.end(); myitem++)
    {
// maybe verify that type is legit here?
        if (myitem->forcewrite || *(myitem->currvalue) != myitem->prevvalue)
        {
            cmd = myitem->edgecmd;
            cmd += " ";
            cmd += myitem->currvalue->getString();
            status = this->rcs->send_doug_command(cmd, 0x0, 0x0);
            myitem->prevvalue.setToValue(*(myitem->currvalue));
            myitem->forcewrite = false;
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
        if (myitem->currvalue->getPointer() == value) myitem->forcewrite = true;
    }
}

int EdgeCommModule::addParameter(int bufID, const char *paramname, const char *cmdspec)
{
    if (!paramname || !cmdspec) return this->Fail;

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

    Variable *myvalue = getVariable(paramname);

    if (myvalue)
    {
// maybe a more elegant "if valid" check below?
        if (myvalue->getPointer())
        {
            io_parameter myparam;
            myparam.edgecmd = cmdspec;
            myparam.type = myvalue->getType();
            myparam.currvalue = myvalue;
            myparam.forcewrite = false;
            io_map->push_back(myparam);

            return this->Success;
        }
    }

    return this->Fail;
}

int EdgeCommModule::finishInitialization(const char *host, const char *port, double spec_rate)
{
    if (this->rcs->initialize(host, port)) return this->Fail;
    this->update_rate = spec_rate;
    this->edge_timer->restart();
    return this->Success;
}

void EdgeCommModule::activate(void)
{
    int ret;
    std::string basecmd, cmd;

    if (!(this->fromEdge.empty()) || !(this->toEdge.empty()))
    {
        TIDY(this->cmd_group);
        cmd = "create_command_group";
        if (this->rcs->send_doug_command(cmd, &(this->cmd_group), 0x0)) return;
        if (!(this->cmd_group)) return;
        if (!(this->cmd_group[0])) return;

        basecmd = "add_command_to_group ";
        basecmd += this->cmd_group;
        basecmd += " \"";

        std::list<io_parameter>::iterator myitem;
        for (myitem = this->fromEdge.begin(); myitem != this->fromEdge.end(); myitem++)
        {
            cmd = basecmd;
            cmd += myitem->edgecmd;
            cmd += "\"";
            ret = this->rcs->send_doug_command(cmd, 0x0, 0x0);
            if (ret) return;
        }

        this->active = true;
    }
}

bool EdgeCommModule::isActive(void)
{
    return this->active;
}
