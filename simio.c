#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simio_constants.h"

#define ALLOCATION_CHUNK 10

typedef struct
{
	char *label;
	int type;
	void *value;
} parameter;

typedef struct
{
	int count;
    int allocated_elements;
	parameter *data;
} parameter_list;

static parameter *get_paramdata(const char *);

static parameter_list params;


void simio_initialize_parameter_list(void)
{
    params.count = 0;
    params.allocated_elements = 0;
}

int simio_add_parameter(const char *paramname, const char *typestr, const char *initval)
{
    if (params.count == params.allocated_elements)
    {
        params.allocated_elements += ALLOCATION_CHUNK;
        params.data = realloc(params.data, params.allocated_elements * sizeof(parameter));
        if (params.data == NULL) return SIMIO_ERROR;
    }

    params.data[params.count].label = strdup(paramname);
    if (!strcmp(typestr, "Float"))
    {
        params.data[params.count].type = SIMIO_FLOAT;
        params.data[params.count].value = calloc(1, sizeof(float));
        if (initval != NULL) *(float *)params.data[params.count].value = strtof(initval, NULL);
    }
    else if (!strcmp(typestr, "Integer"))
    {
        params.data[params.count].type = SIMIO_INTEGER;
        params.data[params.count].value = calloc(1, sizeof(int));
        if (initval != NULL) *(int *)params.data[params.count].value = strtol(initval, NULL, 10);
    }
    else if (!strcmp(typestr, "String"))
    {
        params.data[params.count].type = SIMIO_STRING;
        params.data[params.count].value = calloc(STRING_DEFAULT_LENGTH, sizeof(char));
        if (initval != NULL) strcpy((char *)params.data[params.count].value, initval);
    }
    else
    {
        params.data[params.count].type = SIMIO_UNKNOWN_TYPE;
    }

    params.count++;

    return 0;
}

void *get_pointer(const char *label)
{
	parameter *myparam = get_paramdata(label);
	if (myparam == NULL)
	{
		printf("get_pointer: invalid parameter label: %s\n", label);
		return NULL;
	}
	else
		return myparam->value;
}

int get_datatype(const char *label)
{
	parameter *myparam = get_paramdata(label);
	if (myparam == NULL)
	{
		printf("get_datatype: invalid parameter label: %s\n", label);
		return SIMIO_UNKNOWN_TYPE;
	}
	else
		return myparam->type;
}

void simio_term(void)
{
	int i;

	for (i=0; i<params.count; i++)
	{
		if (params.data[i].type)
		{
			free(params.data[i].label);
			free(params.data[i].value);
		}
	}
	free(params.data);
}

static parameter *get_paramdata(const char *label)
{
	int i;
	for (i=0; i<params.count; i++)
	{
		if (!strcmp(params.data[i].label, label)) return &params.data[i];
	}
	return NULL;
}
