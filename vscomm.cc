/*******************************************************************************
Description: Library that facilitates communication between real-time data
    monitoring applications and the Trick variable server.
Programmer: M. McFarlane, March 2005
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/utsname.h>
#include "trick_utils/comm/include/tc.h"
#include "trick_utils/comm/include/tc_proto.h"
#include "vscomm_constants.hh"

#define TIDY(a) if (a != NULL) { free(a); a=NULL; }

typedef struct paramarray
{
    struct paramarray *next;
    char *param;
    char *units;
    int type;
    int nelem;
    void *value;
} ParamArray;

static void sim_read(void);
static void sim_write_const(const char *);
static void sim_write(char *);
static int update_data(char *);
static int count_tokens(const char *, char);
static int find_next_token(const char *, char);
static void clean_paramarray(void);

static TCDevice connection;
static ParamArray *parray = NULL;
static char *databuf = NULL, *prevbuf = NULL;
static int databuf_complete = 0, databuf_size = 0, paramcount = 0;

void *vscomm_add_var(char *param, char *units, int type, int nelem)
{
    ParamArray *pstruct, *pnew;

    for (pstruct = parray; pstruct != NULL; pstruct = pstruct->next)
    {
        if (!strcmp(pstruct->param, param) &&
            !strcmp(pstruct->units, units)) return pstruct->value;
    }

    pnew = (ParamArray *)calloc(1, sizeof(ParamArray));
    pnew->param = strdup(param);
    pnew->units = strdup(units);
    pnew->type = type;
    pnew->nelem = nelem;
    pnew->value = NULL;
    pnew->next = NULL;

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

    if (parray == NULL) parray = pnew;
    else
    {
        for (pstruct = parray; pstruct->next != NULL; pstruct = pstruct->next);
        pstruct->next = pnew;
    }

    paramcount++;

    return pnew->value;
}

int vscomm_remove_var(char *param)
{
    char *cmd=NULL;
    ParamArray *pstruct, *prev=NULL;

    if (asprintf(&cmd, "trick.var_remove(\"%s\")\n", param) == -1) return VS_ERROR;

    sim_write(cmd);
    free(cmd);

    for (pstruct = parray; pstruct != NULL; pstruct = prev->next)
    {
        if (!strcmp(pstruct->param, param))
        {
            if (!prev) parray = pstruct->next;
            else prev->next = pstruct->next;
            TIDY(pstruct->param);
            TIDY(pstruct->units);
            TIDY(pstruct->value);
            TIDY(pstruct);
        }
        else prev = pstruct;
    }

    paramcount--;

    return VS_SUCCESS;
}

int vscomm_activate(char *host, int port, char *rate_spec, char *default_rate)
{
    char *cmd=NULL, *sample_rate=NULL, *default_sample_rate = strdup(VS_DEFAULT_SAMPLERATE);
    int i;
    ParamArray *pstruct;

    bzero(&connection, sizeof(TCDevice));

    if (host)
    {
        if (strlen(host)) connection.hostname = strdup(host);
    }
    if (!(connection.hostname)) connection.hostname = strdup("localhost");

    if (port) connection.port = port;
    else connection.port = VS_DEFAULT_PORT;

    connection.disable_handshaking = TC_COMM_TRUE;

    // disable error messages
    tc_error(&connection, 0);

    if (tc_connect(&connection) != TC_SUCCESS) 
    {
        TIDY(connection.hostname);
        return VS_INVALID_CONNECTION;
    }

#if 0
    int new_bytes;
    char *trickver;
    sim_write_const("trick.var_add(\"trick_sys.sched.current_version\")\n");
    sim_write_const("trick.var_send()\n");
    while (!tc_pending(&connection)) continue;
    new_bytes = tc_pending(&connection);
    databuf = (char *)malloc(new_bytes);
    tc_read(&connection, databuf, new_bytes);
    sim_write_const("trick.var_clear()\n");
    trickver = (char *)malloc(new_bytes);
    sscanf(databuf, "0\t%s\n", trickver);
    printf("dcapp: Trick Version = %s\n", trickver);
    free(trickver);
#endif

    if (rate_spec)
    {
        if (asprintf(&cmd, "trick.var_add(\"%s\")\n", rate_spec) == -1) return VS_ERROR;
        sim_write(cmd);
        TIDY(cmd);
        sim_write_const("trick.var_send()\n");

        // block until sim returns rate_spec
        while (!databuf_complete) sim_read();

        // extract data up until the first white space
        for (i=0; i<databuf_size; i++)
        {
            if (isspace(databuf[i]))
            {
                databuf[i] = '\0';
                break;
            }
        }
        sample_rate = databuf + find_next_token(databuf, '\t') + 1;
        sim_write_const("trick.var_clear()\n");
    }
    else
    {
        if (default_rate)
        {
            if (strlen(default_rate)) sample_rate = default_rate;
        }
        if (!sample_rate) sample_rate = default_sample_rate;
    }

    sim_write_const("trick.var_pause()\n");

    for (pstruct = parray; pstruct != NULL; pstruct = pstruct->next)
    {
        if (strcmp(pstruct->units, "--"))
        {
            if (asprintf(&cmd, "trick.var_add(\"%s\", \"%s\")\n", pstruct->param, pstruct->units) == -1) return VS_ERROR;
        }
        else
        {
            if (asprintf(&cmd, "trick.var_add(\"%s\")\n", pstruct->param) == -1) return VS_ERROR;
        }
        sim_write(cmd);
        TIDY(cmd);
    }

    if (asprintf(&cmd, "trick.var_cycle(%s)\n", sample_rate) == -1) return VS_ERROR;
    sim_write(cmd);

//    sim_write_const("trick.var_debug(1)\n"); // FOR DEBUGGING
    sim_write_const("trick.var_unpause()\n");

    TIDY(cmd);
    TIDY(default_sample_rate);
    
    return VS_SUCCESS;
}

// this routine processes one buffer (up to the first \n) and ignores all data after the final \n
// NOTE: this means that if multiple buffers are received, only one will be processed
int vscomm_get(void)
{
    int end_buf, retval;

    if (!tc_isValid(&connection)) return VS_INVALID_CONNECTION;

    sim_read();

    if (databuf == 0 || databuf_size == 0) return VS_NO_NEW_DATA;
    if (!databuf_complete) return VS_PARTIAL_BUFFER;

    end_buf = find_next_token(databuf, '\n');
    databuf_size = end_buf + 1;

    if (count_tokens(databuf, '\t') != paramcount) return VS_MANGLED_BUFFER;

    databuf[end_buf] = '\t'; // replace \n with \t to simplify parsing
    retval = update_data(databuf);

    return retval;
}

int vscomm_put(char *param, int type, void *value, char *units)
{
    char *cmd;

    if (!tc_isValid(&connection)) return VS_INVALID_CONNECTION;

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
    }

    sim_write(cmd);
    free(cmd);
    return VS_SUCCESS;
}

void vscomm_terminate(void)
{
    if (connection.disabled == TC_COMM_ENABLED)
    {
        sim_write_const("trick.var_clear()\n");
        sim_write_const("trick.var_exit()\n");
        tc_disconnect(&connection);
    }
    clean_paramarray();
    TIDY(connection.hostname);
    TIDY(databuf);
    TIDY(prevbuf);
}

static void sim_read(void)
{
    int new_bytes;

    if (databuf_complete)
    {
        databuf_size = 0;
        databuf_complete = 0;
    }

    while ((new_bytes = tc_pending(&connection)))
    {
        databuf = (char *)realloc(databuf, databuf_size + new_bytes);
        tc_read(&connection, &databuf[databuf_size], new_bytes);
        databuf_size += new_bytes;
    }

    if (count_tokens(databuf, '\n')) databuf_complete = 1;
}

static void sim_write_const(const char *cmd)
{
    char *dyncmd = strdup(cmd);
    tc_write(&connection, dyncmd, strlen(cmd));
    TIDY(dyncmd);
}

static void sim_write(char *cmd)
{
    tc_write(&connection, cmd, strlen(cmd));
}

static int update_data(char *curbuf)
{
    ParamArray *pstruct;
    char *element;
    int len;

    prevbuf = (char *)realloc(prevbuf, databuf_size);

    if (!strncmp(prevbuf, curbuf, databuf_size)) return VS_NO_NEW_DATA;

    bcopy(curbuf, prevbuf, databuf_size);

    for (pstruct = parray, element=curbuf; pstruct != NULL; pstruct = pstruct->next)
    {
        element += find_next_token(element, '\t') + 1;
        switch (pstruct->type)
        {
        case VS_FLOAT:
            *(float *)pstruct->value = strtof(element, NULL);
            break;
        case VS_INTEGER:
            *(int *)pstruct->value = (int)strtol(element, NULL, 10);
            break;
        case VS_STRING:
            len = find_next_token(element, '\t');
            strncpy((char *)pstruct->value, element, len);
            *((char *)(pstruct->value)+len) = '\0';
            break;
        }
    }

    return VS_SUCCESS;
}

static int count_tokens(const char *str, char key)
{
    int i, count=0;
    
    for (i=0; i<databuf_size; i++)
    {
        if (str[i] == key) count++;
    }
    
    return count;
}

static int find_next_token(const char *str, char key)
{
    int i;
    
    for (i=0; i<databuf_size; i++)
    {
        if (str[i] == key) return i;
    }
    
    return (-1);
}

static void clean_paramarray(void)
{
    ParamArray *pstruct, *tmp;

    for (pstruct = parray; pstruct != NULL;)
    {
        tmp = pstruct->next;
        TIDY(pstruct->param);
        TIDY(pstruct->units);
        TIDY(pstruct->value);
        TIDY(pstruct);
        pstruct = tmp;
    }
    
    parray = NULL;
}
