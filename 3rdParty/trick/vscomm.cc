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
#include "vscomm.hh"

#define VS_DEFAULT_PORT       7000
#define VS_DEFAULT_SAMPLERATE "1.0"

ValueData::ValueData() : decval(0), intval(0) { }

ValueData::~ValueData() { }

ParamData::ParamData(const char *label_spec, const char *units_spec, int type_spec)
{
    if (label_spec) this->label = label_spec;
    if (units_spec) this->units = units_spec;
    this->type = type_spec;
}

ParamData::~ParamData() { }

void ParamData::setValue(const char *input, unsigned length)
{
    switch (this->type)
    {
    case VS_DECIMAL:
        this->value.decval = strtod(input, 0x0);
        break;
    case VS_INTEGER:
        this->value.intval = (int)strtol(input, 0x0, 10);
        break;
    case VS_STRING:
        if (length >= this->value.strval.max_size()) length = this->value.strval.max_size() - 1;
        this->value.strval.clear();
        for (unsigned i=0; i<length; i++) this->value.strval += input[i];
        break;
    }
}

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

ValueData * VariableServerComm::add_var(const char *label, const char *units, int type)
{
    if (!label) return nullptr;

    ParamData *newparam = new ParamData(label, units, type);

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

void VariableServerComm::remove_var(const char *label)
{
    for (std::list<ParamData>::iterator it = this->paramlist.begin(); it != this->paramlist.end(); it++)
    {
        if (it->label == label) this->paramlist.erase(it);
    }

    std::ostringstream mycmd;
    mycmd << "trick.var_remove(\"" << label << "\")\n";
    this->sim_write(mycmd.str().c_str());
}

int VariableServerComm::activate(const char *host, int port, const char *rate_spec, char *default_rate)
{
    char *cmd=0x0, *sample_rate=0x0, *default_sample_rate = strdup(VS_DEFAULT_SAMPLERATE);
    int i;

    if (host)
    {
        if (strlen(host)) this->connection.hostname = strdup(host);
    }
    if (!(this->connection.hostname)) this->connection.hostname = strdup("localhost");

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

    if (rate_spec)
    {
        std::ostringstream mycmd;
        mycmd << "trick.var_add(\"" << rate_spec << "\")\n";
        this->sim_write(mycmd.str().c_str());
        this->sim_write("trick.var_send()\n");

        // block until sim returns rate_spec
        while (!(this->databuf_complete)) this->sim_read();

        // extract data up until the first white space
        for (i=0; i<this->databuf_size; i++)
        {
            if (isspace(this->databuf[i]))
            {
                this->databuf[i] = '\0';
                break;
            }
        }
        sample_rate = this->databuf + this->find_next_token(this->databuf, '\t') + 1;
        this->sim_write("trick.var_clear()\n");
    }
    else
    {
        if (default_rate)
        {
            if (strlen(default_rate)) sample_rate = default_rate;
        }
        if (!sample_rate) sample_rate = default_sample_rate;
    }

    this->sim_write("trick.var_pause()\n");

    for (std::list<ParamData>::iterator it = this->paramlist.begin(); it != this->paramlist.end(); it++)
    {
        std::ostringstream mycmd;
        mycmd << "trick.var_add(\"" << it->label << "\"";
        if (!(it->units.empty())) mycmd << ", \"" << it->units << "\"";
        mycmd << ")\n";
        this->sim_write(mycmd.str().c_str());
        TIDY(cmd);
    }

    std::ostringstream mycmd;
    mycmd << "trick.var_cycle(" << sample_rate << ")\n";
    this->sim_write(mycmd.str().c_str());

//    this->sim_write("trick.var_debug(1)\n"); // FOR DEBUGGING
    this->sim_write("trick.var_unpause()\n");

    TIDY(cmd);
    TIDY(default_sample_rate);

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

int VariableServerComm::putMethod(const char *label)
{
    if (!tc_isValid(&(this->connection))) return VS_INVALID_CONNECTION;

    std::ostringstream mycmd;
    mycmd << label << "()\n";
    this->sim_write(mycmd.str().c_str());

    return VS_SUCCESS;
}

int VariableServerComm::putGeneric(const char *label, std::string value, const char *units)
{
    if (!tc_isValid(&(this->connection))) return VS_INVALID_CONNECTION;

    std::ostringstream mycmd;
    mycmd << "trick.var_set(\"" << label << "\", " << value;
    if (units) mycmd << ", \"" << units << "\"";
    mycmd << ")\n";
    this->sim_write(mycmd.str().c_str());

    return VS_SUCCESS;
}

int VariableServerComm::putValue(const char *label, int value, const char *units)
{
    return this->putGeneric(label, std::to_string(value), units);
}

int VariableServerComm::putValue(const char *label, double value, const char *units)
{
    return this->putGeneric(label, std::to_string(value), units);
}

int VariableServerComm::putValue(const char *label, std::string value, const char *)
{
    return this->putGeneric(label, value, 0x0);
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

// If tc_write is ever updated to take "const char *" as input, then this ugly work-around can be removed and sim_write
// can be effectively replaced by a single call to tc_write:  tc_write(&(this->connection), cmd, strlen(cmd));
void VariableServerComm::sim_write(const char *cmd)
{
    char *dyncmd = strdup(cmd);
    if (dyncmd)
    {
        tc_write(&(this->connection), dyncmd, strlen(cmd));
        free(dyncmd);
    }
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
        it->setValue(element, this->find_next_token(element, '\t'));
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
