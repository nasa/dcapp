#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "varlist_constants.hh"

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


void varlist_init(void)
{
    params.count = 0;
    params.allocated_elements = 0;
}

int varlist_append(const char *paramname, const char *typestr, const char *initval)
{
    if (params.count == params.allocated_elements)
    {
        params.allocated_elements += ALLOCATION_CHUNK;
        params.data = (parameter *)realloc(params.data, params.allocated_elements * sizeof(parameter));
        if (params.data == NULL) return VARLIST_ERROR;
    }

    params.data[params.count].label = strdup(paramname);
    if (!strcmp(typestr, "Float"))
    {
        params.data[params.count].type = VARLIST_FLOAT;
        params.data[params.count].value = calloc(1, sizeof(float));
        if (initval != NULL) *(float *)params.data[params.count].value = strtof(initval, NULL);
    }
    else if (!strcmp(typestr, "Integer"))
    {
        params.data[params.count].type = VARLIST_INTEGER;
        params.data[params.count].value = calloc(1, sizeof(int));
        if (initval != NULL) *(int *)params.data[params.count].value = strtol(initval, NULL, 10);
    }
    else if (!strcmp(typestr, "String"))
    {
        params.data[params.count].type = VARLIST_STRING;
        params.data[params.count].value = calloc(STRING_DEFAULT_LENGTH, sizeof(char));
        if (initval != NULL) strcpy((char *)params.data[params.count].value, initval);
    }
    else
    {
        params.data[params.count].type = VARLIST_UNKNOWN_TYPE;
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
		return VARLIST_UNKNOWN_TYPE;
	}
	else
		return myparam->type;
}

void varlist_term(void)
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
