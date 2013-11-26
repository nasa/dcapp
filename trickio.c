#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TRICKACTIVE
#include "vscomm.h"
#endif
#include "simio.h"
#include "trickio_constants.h"

#define ALLOCATION_CHUNK 10

typedef struct
{
	int type;
	char *trickvar;
	char *units;
	void *trickvalue;
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

static io_parameter_list fromsim, tosim;


void trickio_initialize_parameter_list(int bufID)
{
    io_parameter_list *io_map;

    switch (bufID)
    {
        case TRICKIO_FROMTRICK:
            io_map = &fromsim;
            break;
        case TRICKIO_TOTRICK:
            io_map = &tosim;
            break;
        default:
            return;
    }
    io_map->count = 0;
    io_map->allocated_elements = 0;
    io_map->data = NULL;
}

int trickio_add_parameter(int bufID, const char *paramname, const char *trickvar, const char *units)
{
#ifdef TRICKACTIVE
    io_parameter_list *io_map;
    void *valptr;

    switch (bufID)
    {
        case TRICKIO_FROMTRICK:
            io_map = &fromsim;
            break;
        case TRICKIO_TOTRICK:
            io_map = &tosim;
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
            io_map->data = realloc(io_map->data, io_map->allocated_elements * sizeof(io_parameter));
            if (io_map->data == NULL) return SIMIO_ERROR;
        }
        io_map->data[io_map->count].trickvar = strdup(trickvar);

        if (units) io_map->data[io_map->count].units = strdup(units);
        else  io_map->data[io_map->count].units = strdup("--");

        io_map->data[io_map->count].type = get_datatype(paramname);
        io_map->data[io_map->count].dcvalue = valptr;
        io_map->data[io_map->count].prevvalue.i = 0;
        io_map->data[io_map->count].prevvalue.f = 0;
        bzero(io_map->data[io_map->count].prevvalue.str, STRING_DEFAULT_LENGTH);
        io_map->data[io_map->count].forcewrite = 0;
        io_map->count++;
    }
#endif
    
    return 0;
}

int trickio_finish_initialization(void)
{
#ifdef TRICKACTIVE
	int i, type;

	for (i=0; i<fromsim.count; i++)
    {
        switch (fromsim.data[i].type)
        {
				case SIMIO_FLOAT:
					type = VS_FLOAT;
					break;
				case SIMIO_INTEGER:
					type = VS_INTEGER;
					break;
				case SIMIO_STRING:
					type = VS_STRING;
					break;
        }
        fromsim.data[i].trickvalue = vscomm_add_var(fromsim.data[i].trickvar, fromsim.data[i].units, type, 1);
    }
#endif

	return SIMIO_SUCCESS;
}

int trickio_activatecomm(char *host, int port, char *default_rate)
{
#ifdef TRICKACTIVE
    if (fromsim.count == 0 && tosim.count == 0) return SIMIO_NO_DATA_REQUESTED;
	else return vscomm_activate(host, port, NULL, default_rate);
#else
    return SIMIO_NO_DATA_REQUESTED;
#endif
}

int trickio_readsimdata(void)
{
#ifdef TRICKACTIVE
	int i;
	int status = vscomm_get();

	if (status == VS_SUCCESS)
	{
		for (i=0; i<fromsim.count; i++)
		{
			switch (fromsim.data[i].type)
			{
				case SIMIO_FLOAT:
					*(float *)fromsim.data[i].dcvalue = *(float *)fromsim.data[i].trickvalue;
					break;
				case SIMIO_INTEGER:
					*(int *)fromsim.data[i].dcvalue = *(int *)fromsim.data[i].trickvalue;
					break;
				case SIMIO_STRING:
					strcpy((char *)fromsim.data[i].dcvalue, (char *)fromsim.data[i].trickvalue);
					break;
			}
		}
	}

	return status;
#else
    return SIMIO_NO_NEW_DATA;
#endif
}

int trickio_writesimdata(void)
{
#ifdef TRICKACTIVE
	int i, status;

	for (i=0; i<tosim.count; i++)
	{
		switch (tosim.data[i].type)
		{
			case SIMIO_FLOAT:
                if (tosim.data[i].forcewrite || *(float *)tosim.data[i].dcvalue != tosim.data[i].prevvalue.f)
                {
                    status = vscomm_put(tosim.data[i].trickvar, VS_FLOAT, tosim.data[i].dcvalue, tosim.data[i].units);
                    tosim.data[i].prevvalue.f = *(float *)tosim.data[i].dcvalue;
                    tosim.data[i].forcewrite = 0;
                }
				break;
			case SIMIO_INTEGER:
                if (tosim.data[i].forcewrite || *(int *)tosim.data[i].dcvalue != tosim.data[i].prevvalue.i)
                {
                    status = vscomm_put(tosim.data[i].trickvar, VS_INTEGER, tosim.data[i].dcvalue, tosim.data[i].units);
                    tosim.data[i].prevvalue.i = *(int *)tosim.data[i].dcvalue;
                    tosim.data[i].forcewrite = 0;
                }
				break;
			case SIMIO_STRING:
                if (tosim.data[i].forcewrite || strcmp((char *)tosim.data[i].dcvalue, tosim.data[i].prevvalue.str))
                {
                    status = vscomm_put(tosim.data[i].trickvar, VS_STRING, tosim.data[i].dcvalue, tosim.data[i].units);
                    strcpy(tosim.data[i].prevvalue.str, (char *)tosim.data[i].dcvalue);
                    tosim.data[i].forcewrite = 0;
                }
				break;
		}
	}

	return status;
#else
    return SIMIO_NO_NEW_DATA;
#endif
}

void trickio_forcewrite(void *value)
{
#ifdef TRICKACTIVE
	int i;

	for (i=0; i<tosim.count; i++)
	{
        if (tosim.data[i].dcvalue == value) tosim.data[i].forcewrite = 1;
	}
#endif
}

void trickio_term(void)
{
#ifdef TRICKACTIVE
	int i;

	for (i=0; i<fromsim.count; i++) free(fromsim.data[i].trickvar);
	free(fromsim.data);

	for (i=0; i<tosim.count; i++) free(tosim.data[i].trickvar);
	free(tosim.data);

	vscomm_terminate();
#endif
}
