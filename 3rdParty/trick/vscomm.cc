#ifdef TRICKACTIVE

/*******************************************************************************
Description: Library that facilitates communication between real-time data
    monitoring applications and the Trick variable server.
Programmer: M. McFarlane
*******************************************************************************/

#include <list>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <strings.h>
#include "basicutils/tidy.hh"
#include "variables.hh"
#include "vscomm.hh"

#define VS_DEFAULT_PORT       7000
#define VS_DEFAULT_SAMPLERATE "1.0"

ParamData::ParamData(std::string &label_spec, std::string &units_spec, Variable &invar)
{
    this->label = label_spec;
    this->units = units_spec;
    this->value.setAttributes(invar);
}

ParamData::~ParamData() { }

VariableServerComm::VariableServerComm() : databuf(0x0), prevbuf(0x0), databuf_complete(false), databuf_size(0)
{
    bzero(&(this->connection), sizeof(TCDevice));
}

VariableServerComm::~VariableServerComm()
{
    if (this->connection.disabled == TC_COMM_ENABLED)
    {
        this->sim_write("trick.var_clear()\n");
        this->sim_write("trick.var_exit()\n");
        tc_disconnect(&(this->connection));
    }

    TIDY(this->connection.hostname);
    TIDY(this->databuf);
    TIDY(this->prevbuf);
}

Variable * VariableServerComm::add_var(std::string &label, std::string &units, Variable &invar)
{
    if (label.empty()) return nullptr;

    ParamData *newparam = new ParamData(label, units, invar);

    for (std::list<ParamData>::iterator it = this->paramlist.begin(); it != this->paramlist.end(); it++)
    {
        if (newparam->label == it->label && newparam->units == it->units)
        {
            delete newparam;
            return &(it->value);
        }
    }

    this->paramlist.push_back(*newparam);
    return &(this->paramlist.back().value);
}

void VariableServerComm::remove_var(std::string &label)
{
    for (std::list<ParamData>::iterator it = this->paramlist.begin(); it != this->paramlist.end(); it++)
    {
        if (it->label == label) this->paramlist.erase(it);
    }

    std::ostringstream mycmd;
    mycmd << "trick.var_remove(\"" << label << "\")\n";
    this->sim_write(mycmd.str());
}

int VariableServerComm::activate(std::string &host, int port, std::string &datarate)
{
    if (!(this->connection.hostname))
    {
        if (host.empty()) this->connection.hostname = strdup("localhost");
        else this->connection.hostname = strdup(host.c_str());
    }

    if (port) this->connection.port = port;
    else this->connection.port = VS_DEFAULT_PORT;

    this->connection.disable_handshaking = TC_COMM_TRUE;

    // disable error messages
    tc_error(&(this->connection), 0);

    if (tc_connect(&(this->connection)) != TC_SUCCESS)
    {
        TIDY(this->connection.hostname);
        return VS_INVALID_CONNECTION;
    }

#if 0
    int new_bytes;
    char *trickver;
    this->sim_write("trick.var_add(\"trick_sys.sched.current_version\")\n");
    this->sim_write("trick.var_send()\n");
    while (!tc_pending(&(this->connection))) continue;
    new_bytes = tc_pending(&(this->connection));
    this->databuf = (char *)malloc(new_bytes);
    tc_read(&(this->connection), this->databuf, new_bytes);
    this->sim_write("trick.var_clear()\n");
    trickver = (char *)malloc(new_bytes);
    sscanf(this->databuf, "0\t%s\n", trickver);
    user_msg("Trick Version = " << trickver);
    free(trickver);
#endif

    this->sim_write("trick.var_pause()\n");

    for (std::list<ParamData>::iterator it = this->paramlist.begin(); it != this->paramlist.end(); it++)
    {
        std::ostringstream mycmd;
        mycmd << "trick.var_add(\"" << it->label << "\"";
        if (!(it->units.empty())) mycmd << ", \"" << it->units << "\"";
        mycmd << ")\n";
        this->sim_write(mycmd.str());
    }

    std::ostringstream mycmd;
    if (datarate.empty()) mycmd << "trick.var_cycle(" << VS_DEFAULT_SAMPLERATE << ")\n";
    else mycmd << "trick.var_cycle(" << datarate << ")\n";
    this->sim_write(mycmd.str());

//    this->sim_write("trick.var_debug(1)\n"); // FOR DEBUGGING
    this->sim_write("trick.var_unpause()\n");

    return VS_SUCCESS;
}

// this routine processes one buffer (up to the first \n) and ignores all data after the final \n
// NOTE: this means that if multiple buffers are received, only one will be processed
int VariableServerComm::get(void)
{
    int end_buf, retval, store_start, store_bytes;

    if (!tc_isValid(&(this->connection))) return VS_INVALID_CONNECTION;

    this->sim_read();

    if (!(this->databuf) || this->databuf_size == 0) return VS_NO_NEW_DATA;

    if (!(this->databuf_complete)) return VS_PARTIAL_BUFFER;

    // if there's a partial buffer after the last \n, mark its location
    store_start = this->find_last_token(this->databuf, '\n') + 1;
    store_bytes = this->databuf_size - store_start;

    end_buf = this->find_next_token(this->databuf, '\n');
    this->databuf_size = end_buf + 1;

    if (this->count_tokens(this->databuf, '\t') != this->paramlist.size()) return VS_MANGLED_BUFFER;

    this->databuf[end_buf] = '\t'; // replace \n with \t to simplify parsing
    retval = this->update_data(this->databuf);

    // if there's a partial buffer after the last \n, shift the bytes to the front of the buffer
    if (store_bytes > 0)
    {
        char *store_buf = &(this->databuf[store_start]);

        for (int i=0; i<store_bytes; i++)
        {
            this->databuf[i] = store_buf[i];
        }

        this->databuf_size = store_bytes;
        this->databuf_complete = false;
    }

    return retval;
}

int VariableServerComm::putMethod(std::string &label)
{
    if (!tc_isValid(&(this->connection))) return VS_INVALID_CONNECTION;

    std::ostringstream mycmd;
    mycmd << label << "()\n";
    this->sim_write(mycmd.str());

    return VS_SUCCESS;
}

int VariableServerComm::putValue(std::string &label, Variable &value, std::string &units)
{
    if (!tc_isValid(&(this->connection))) return VS_INVALID_CONNECTION;

    std::ostringstream mycmd;
    mycmd << "trick.var_set(\"" << label << "\", " << value.getString();
    if (!(units.empty()) && !(value.isString())) mycmd << ", \"" << units << "\"";
    mycmd << ")\n";
    this->sim_write(mycmd.str());

    return VS_SUCCESS;
}

void VariableServerComm::sim_read(void)
{
    int new_bytes;

    if (this->databuf_complete)
    {
        this->databuf_size = 0;
        this->databuf_complete = false;
    }

    while ((new_bytes = tc_pending(&(this->connection))))
    {
        this->databuf = (char *)realloc(this->databuf, this->databuf_size + new_bytes);
        tc_read(&(this->connection), &(this->databuf[databuf_size]), new_bytes);
        this->databuf_size += new_bytes;
    }

    if (this->count_tokens(this->databuf, '\n')) this->databuf_complete = true;
}

// The const_cast below is a bit of a hack because tc_write requires "char *" as its 2nd argument.  The const_cast
// allows tc_write to change the contents of cmd, which may be dangerous.  This hack will go away if tc_write is ever
// updated to accept "const char *" as its 2nd argument.
void VariableServerComm::sim_write(std::string cmd)
{
    tc_write(&(this->connection), const_cast<char *>(cmd.c_str()), cmd.length());
}

int VariableServerComm::update_data(const char *curbuf)
{
    this->prevbuf = (char *)realloc(this->prevbuf, this->databuf_size);

    if (!strncmp(this->prevbuf, curbuf, this->databuf_size)) return VS_NO_NEW_DATA;

    bcopy(curbuf, this->prevbuf, this->databuf_size);

    const char *element = curbuf;

    for (std::list<ParamData>::iterator it = this->paramlist.begin(); it != this->paramlist.end(); it++)
    {
        element += this->find_next_token(element, '\t') + 1;
        std::string tmpstr;
        tmpstr.assign(element, this->find_next_token(element, '\t'));
        it->value.setToString(tmpstr);
    }

    return VS_SUCCESS;
}

size_t VariableServerComm::count_tokens(const char *str, char key)
{
    int count = 0;

    for (int i = 0; i < this->databuf_size; i++)
    {
        if (str[i] == key) count++;
    }

    return count;
}

int VariableServerComm::find_next_token(const char *str, char key)
{
    for (int i = 0; i < this->databuf_size; i++)
    {
        if (str[i] == key) return i;
    }

    return (-1);
}

int VariableServerComm::find_last_token(const char *str, char key)
{
    for (int i = this->databuf_size - 1; i >= 0; i--)
    {
        if (str[i] == key) return i;
    }

    return (-1);
}

#endif
