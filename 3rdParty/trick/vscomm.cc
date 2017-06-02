/*******************************************************************************
Description: Library that facilitates communication between real-time data
    monitoring applications and the Trick variable server.
Programmer: M. McFarlane, March 2005
*******************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <strings.h>
#include <sys/utsname.h>
#include "basicutils/tidy.hh"
#include "vscomm.hh"

VariableServerComm::VariableServerComm()
:
parray(0x0),
databuf(0x0),
prevbuf(0x0),
databuf_complete(false),
databuf_size(0),
paramcount(0)
{
    bzero(&(this->connection), sizeof(TCDevice));
}

VariableServerComm::~VariableServerComm()
{
    ParamArray *pstruct, *tmp;

    if (this->connection.disabled == TC_COMM_ENABLED)
    {
        this->sim_write("trick.var_clear()\n");
        this->sim_write("trick.var_exit()\n");
        tc_disconnect(&(this->connection));
    }

    for (pstruct = this->parray; pstruct;)
    {
        tmp = pstruct->next;
        TIDY(pstruct->param);
        TIDY(pstruct->units);
        TIDY(pstruct->value);
        TIDY(pstruct);
        pstruct = tmp;
    }

    this->parray = 0x0;

    TIDY(this->connection.hostname);
    TIDY(this->databuf);
    TIDY(this->prevbuf);
}

void * VariableServerComm::add_var(const char *param, const char *units, int type, int nelem)
{
    ParamArray *pstruct, *pnew;

    for (pstruct = this->parray; pstruct; pstruct = pstruct->next)
    {
        if (!strcmp(pstruct->param, param) &&
            !strcmp(pstruct->units, units)) return pstruct->value;
    }

    pnew = (ParamArray *)calloc(1, sizeof(ParamArray));
    pnew->param = strdup(param);
    pnew->units = strdup(units);
    pnew->type = type;
    pnew->nelem = nelem;
    pnew->value = 0x0;
    pnew->next = 0x0;

    switch (pnew->type)
    {
        case VS_FLOAT:
            pnew->value = calloc(1, sizeof(float));
            break;
        case VS_INTEGER:
            pnew->value = calloc(1, sizeof(int));
            break;
        case VS_STRING:
            pnew->value = calloc(nelem, sizeof(char));
            break;
    }

    if (!(this->parray)) this->parray = pnew;
    else
    {
        for (pstruct = this->parray; pstruct->next; pstruct = pstruct->next);
        pstruct->next = pnew;
    }

    this->paramcount++;

    return pnew->value;
}

int VariableServerComm::remove_var(const char *param)
{
    char *cmd=0x0;
    ParamArray *pstruct, *prev=0x0;

    if (asprintf(&cmd, "trick.var_remove(\"%s\")\n", param) == -1) return VS_ERROR;

    this->sim_write(cmd);
    free(cmd);

    for (pstruct = this->parray; pstruct; pstruct = prev->next)
    {
        if (!strcmp(pstruct->param, param))
        {
            if (!prev) this->parray = pstruct->next;
            else prev->next = pstruct->next;
            TIDY(pstruct->param);
            TIDY(pstruct->units);
            TIDY(pstruct->value);
            TIDY(pstruct);
        }
        else prev = pstruct;
    }

    this->paramcount--;

    return VS_SUCCESS;
}

int VariableServerComm::activate(const char *host, int port, const char *rate_spec, char *default_rate)
{
    char *cmd=0x0, *sample_rate=0x0, *default_sample_rate = strdup(VS_DEFAULT_SAMPLERATE);
    int i;
    ParamArray *pstruct;

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
        if (asprintf(&cmd, "trick.var_add(\"%s\")\n", rate_spec) == -1) return VS_ERROR;
        this->sim_write(cmd);
        TIDY(cmd);
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

    for (pstruct = this->parray; pstruct; pstruct = pstruct->next)
    {
        if (strcmp(pstruct->units, "--"))
        {
            if (asprintf(&cmd, "trick.var_add(\"%s\", \"%s\")\n", pstruct->param, pstruct->units) == -1) return VS_ERROR;
        }
        else
        {
            if (asprintf(&cmd, "trick.var_add(\"%s\")\n", pstruct->param) == -1) return VS_ERROR;
        }
        this->sim_write(cmd);
        TIDY(cmd);
    }

    if (asprintf(&cmd, "trick.var_cycle(%s)\n", sample_rate) == -1) return VS_ERROR;
    this->sim_write(cmd);

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

    if (this->count_tokens(this->databuf, '\t') != this->paramcount) return VS_MANGLED_BUFFER;

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

int VariableServerComm::put(const char *param, int type, void *value, const char *units)
{
    char *cmd = 0x0;

    if (!tc_isValid(&(this->connection))) return VS_INVALID_CONNECTION;

    switch (type)
    {
        case VS_FLOAT:
            if (asprintf(&cmd, "trick.read_checkpoint_from_string(\"%s {%s} = %f;\")\n", param, units, *(float *)value) == -1) return VS_ERROR;
            break;
        case VS_INTEGER:
            if (asprintf(&cmd, "trick.read_checkpoint_from_string(\"%s {%s} = %d;\")\n", param, units, *(int *)value) == -1) return VS_ERROR;
            break;
        case VS_STRING:
            if (asprintf(&cmd, "trick.read_checkpoint_from_string(\"%s = %s;\")\n", param, (char *)value) == -1) return VS_ERROR;
            break;
        case VS_METHOD:
            if (asprintf(&cmd, "%s()\n", param) == -1) return VS_ERROR;
            break;
    }

    this->sim_write(cmd);
    free(cmd);
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

void VariableServerComm::sim_write(char *cmd)
{
    tc_write(&(this->connection), cmd, strlen(cmd));
}

// If tc_write is ever updated to take "const char *" as input, then this ugly work-around can be removed
void VariableServerComm::sim_write(const char *cmd)
{
    char *dyncmd = strdup(cmd);
    tc_write(&(this->connection), dyncmd, strlen(cmd));
    TIDY(dyncmd);
}

int VariableServerComm::update_data(char *curbuf)
{
    ParamArray *pstruct;
    char *element;
    int len;

    this->prevbuf = (char *)realloc(this->prevbuf, this->databuf_size);

    if (!strncmp(this->prevbuf, curbuf, this->databuf_size)) return VS_NO_NEW_DATA;

    bcopy(curbuf, this->prevbuf, this->databuf_size);

    for (pstruct = this->parray, element=curbuf; pstruct; pstruct = pstruct->next)
    {
        element += this->find_next_token(element, '\t') + 1;
        switch (pstruct->type)
        {
        case VS_FLOAT:
            *(float *)pstruct->value = strtof(element, 0x0);
            break;
        case VS_INTEGER:
            *(int *)pstruct->value = (int)strtol(element, 0x0, 10);
            break;
        case VS_STRING:
            len = this->find_next_token(element, '\t');
            if (len >= pstruct->nelem) len = pstruct->nelem - 1;
            strncpy((char *)pstruct->value, element, len);
            *((char *)(pstruct->value)+len) = '\0';
            break;
        }
    }

    return VS_SUCCESS;
}

int VariableServerComm::count_tokens(const char *str, char key)
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
