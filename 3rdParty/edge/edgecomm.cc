#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "variables.hh"
#include "edgecomm.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

EdgeCommModule::EdgeCommModule() : io_map(0x0), cmd_group(0x0), rcs(0x0)
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
        TIDY(result);
        return this->Fail;
    }

    substr = (char *)calloc(1, strlen(result));
    if (!substr)
    {
        this->active = false;
        TIDY(result);
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
            TIDY(substr);
            TIDY(result);
            return this->Fail;
        }

        myitem->currvalue->setToString(substr);

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

void EdgeCommModule::flagAsChanged(Variable *value)
{
    std::list<io_parameter>::iterator myitem;

    for (myitem = this->toEdge.begin(); myitem != this->toEdge.end(); myitem++)
    {
        if (myitem->currvalue == value) myitem->forcewrite = true;
    }
}

int EdgeCommModule::addParameter(std::string paramname, std::string cmdspec)
{
    if (paramname.empty() || cmdspec.empty() || !io_map) return this->Fail;

    Variable *myvalue = getVariable(paramname);

    if (myvalue)
    {
        io_parameter myparam;
        myparam.edgecmd = cmdspec;
        myparam.currvalue = myvalue;
        myparam.prevvalue.setAttributes(*myvalue);
        myparam.forcewrite = false;
        io_map->push_back(myparam);

        return this->Success;
    }
    else return this->Fail;
}

int EdgeCommModule::finishInitialization(std::string host, std::string port, double spec_rate)
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
