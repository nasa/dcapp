#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "EDGE_rcs.hh"
#include "simio.hh"
#include "edgeio_constants.hh"

#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))
#define TIDY(a) if (a != NULL) { free(a); a=NULL; }

#define ALLOCATION_CHUNK 10

typedef struct
{
	int type;
	char *edgecmd;
	void *dcvalue;
    union
    {
        int i;
        float f;
        char str[STRING_DEFAULT_LENGTH];
    } prevvalue;
    int forcewrite;
} io_parameter;

typedef struct
{
	int count;
    int allocated_elements;
	io_parameter *data;
} io_parameter_list;

static io_parameter_list fromedge, toedge;
static struct timeval edge_timer;
static float update_rate;
static char *cmd_group = NULL;


void edgeio_initialize_parameter_list(int bufID)
{
    io_parameter_list *io_map;

    switch (bufID)
    {
        case EDGEIO_FROMEDGE:
            io_map = &fromedge;
            break;
        case EDGEIO_TOEDGE:
            io_map = &toedge;
            break;
        default:
            return;
    }
    io_map->count = 0;
    io_map->allocated_elements = 0;
    io_map->data = NULL;
}

int edgeio_add_parameter(int bufID, const char *paramname, const char *edgecmd)
{
    io_parameter_list *io_map;
    void *valptr;

    switch (bufID)
    {
        case EDGEIO_FROMEDGE:
            io_map = &fromedge;
            break;
        case EDGEIO_TOEDGE:
            io_map = &toedge;
            break;
        default:
            return SIMIO_ERROR;
    }

    valptr = get_pointer(paramname);

    if (valptr)
    {
        if (io_map->count == io_map->allocated_elements)
        {
            io_map->allocated_elements += ALLOCATION_CHUNK;
            io_map->data = (io_parameter *)realloc(io_map->data, io_map->allocated_elements * sizeof(io_parameter));
            if (io_map->data == NULL) return SIMIO_ERROR;
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
    
    return 0;
}

int edgeio_finish_initialization(char *host, char *port, float spec_rate)
{
    if (EDGE_rcs_init(host, port)) return SIMIO_ERROR;
    update_rate = spec_rate;
    gettimeofday(&edge_timer, NULL);
	return SIMIO_SUCCESS;
}

int edgeio_activatecomm(void)
{
    int i, ret;
    char *cmd = NULL;

    if (fromedge.count == 0 && toedge.count == 0) return SIMIO_NO_DATA_REQUESTED;

    TIDY(cmd_group);
    ret = send_doug_command("create_command_group", &cmd_group, NULL);
    if (ret) return SIMIO_ERROR;
    if (!cmd_group) return SIMIO_ERROR;
    if (!cmd_group[0]) return SIMIO_ERROR;

    for (i=0; i<fromedge.count; i++)
    {
        if (asprintf(&cmd, "add_command_to_group %s \"%s\"", cmd_group, fromedge.data[i].edgecmd) == -1) return SIMIO_ERROR;
        ret = send_doug_command(cmd, NULL, NULL);
        TIDY(cmd);
        if (ret) return SIMIO_ERROR;
    }

    return SIMIO_SUCCESS;
}

int edgeio_readsimdata(void)
{
	int i, ret;
	struct timeval now;
    char *result = NULL;
    char *cmd = NULL, *strptr, *strval = NULL;

    gettimeofday(&now, NULL);

    if (SecondsElapsed(edge_timer, now) < update_rate) return SIMIO_NO_NEW_DATA;

    if (asprintf(&cmd, "execute_command_group %s", cmd_group) == -1) return SIMIO_ERROR;
    ret = send_doug_command(cmd, &result, NULL);
    TIDY(cmd);
    if (ret) return SIMIO_ERROR;

    strval = (char *)calloc(1, strlen(result));
    if (!strval) return SIMIO_ERROR;

    for (i=0, strptr=result; i<fromedge.count; i++)
    {
        ret = sscanf(strptr, "%s", strval);
        if (!ret) return SIMIO_ERROR;

        switch (fromedge.data[i].type)
        {
            case SIMIO_FLOAT:
                *(float *)fromedge.data[i].dcvalue = strtof(strval, NULL);
                break;
            case SIMIO_INTEGER:
                *(int *)fromedge.data[i].dcvalue = (int)strtol(strval, NULL, 10);
                break;
            case SIMIO_STRING:
                strcpy((char *)fromedge.data[i].dcvalue, strval);
                break;
        }

        strptr += strlen(strval) + 1;
    }

    TIDY(strval);
    TIDY(result);

    edge_timer = now;

	return SIMIO_SUCCESS;
}

int edgeio_writesimdata(void)
{
	int i, status = 0;
	char *cmd = NULL;

	for (i=0; i<toedge.count; i++)
	{
		switch (toedge.data[i].type)
		{
			case SIMIO_FLOAT:
                if (toedge.data[i].forcewrite || *(float *)toedge.data[i].dcvalue != toedge.data[i].prevvalue.f)
                {
                    if (asprintf(&cmd, "%s %f", toedge.data[i].edgecmd, *(float *)(toedge.data[i].dcvalue)) == -1) return 1;
                    status = send_doug_command(cmd, NULL, NULL);
                    toedge.data[i].prevvalue.f = *(float *)toedge.data[i].dcvalue;
                    toedge.data[i].forcewrite = 0;
		            TIDY(cmd);
                }
				break;
			case SIMIO_INTEGER:
                if (toedge.data[i].forcewrite || *(int *)toedge.data[i].dcvalue != toedge.data[i].prevvalue.i)
                {
                    if (asprintf(&cmd, "%s %d", toedge.data[i].edgecmd, *(int *)(toedge.data[i].dcvalue)) == -1) return 1;
                    status = send_doug_command(cmd, NULL, NULL);
                    toedge.data[i].prevvalue.i = *(int *)toedge.data[i].dcvalue;
                    toedge.data[i].forcewrite = 0;
		            TIDY(cmd);
                }
				break;
			case SIMIO_STRING:
                if (toedge.data[i].forcewrite || strcmp((char *)toedge.data[i].dcvalue, toedge.data[i].prevvalue.str))
                {
                    if (asprintf(&cmd, "%s %s", toedge.data[i].edgecmd, (char *)(toedge.data[i].dcvalue)) == -1) return 1;
                    status = send_doug_command(cmd, NULL, NULL);
                    strcpy(toedge.data[i].prevvalue.str, (char *)toedge.data[i].dcvalue);
                    toedge.data[i].forcewrite = 0;
		            TIDY(cmd);
                }
				break;
		}
	}

	return status;
}

void edgeio_forcewrite(void *value)
{
	int i;

	for (i=0; i<toedge.count; i++)
	{
        if (toedge.data[i].dcvalue == value) toedge.data[i].forcewrite = 1;
	}
}

void edgeio_term(void)
{
	int i;

	for (i=0; i<fromedge.count; i++) TIDY(fromedge.data[i].edgecmd);
	TIDY(fromedge.data);
	for (i=0; i<toedge.count; i++) TIDY(toedge.data[i].edgecmd);
	TIDY(toedge.data);

    TIDY(cmd_group);

    EDGE_rcs_term();
}
