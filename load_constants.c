#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nodes.h"

extern appdata AppData;


static void *FindConstant(int, void *);

void *LoadConstant(int datatype, void *value)
{
    void *myptr;
    struct node *data;

    // check if this constant has already been loaded
    myptr = FindConstant(datatype, value);

    // if not, create a new constant node and load the constant
    if (myptr == 0)
    {
        data = NewNode(NULL, &(AppData.ConstantList));
        data->object.constants.datatype = datatype;
        switch (datatype)
        {
            case FLOAT:
                data->object.constants.val.f = *(float *)value;
                myptr = &(data->object.constants.val.f);
                break;
            case INTEGER:
                data->object.constants.val.i = *(int *)value;
                myptr = &(data->object.constants.val.i);
                break;
            case STRING:
                data->object.constants.val.s = strdup((char *)value);
                myptr = data->object.constants.val.s;
            default:
                break;
        }
    }

    return myptr;
}

/*********************************************************************************
 *
 * This function will determine if a texture file has already been loaded.
 *
 *********************************************************************************/
static void *FindConstant(int datatype, void *value)
{
    struct node *current;

    // Traverse the list to find the constant
    for (current = AppData.ConstantList; current != NULL; current = current->p_next)
    {
        // If we find it, return the pointer
        if (current->object.constants.datatype == datatype)
        switch (datatype)
        {
            case FLOAT:
                if (current->object.constants.val.f == *(float *)value) return &(current->object.constants.val.f);
                break;
            case INTEGER:
                if (current->object.constants.val.i == *(int *)value) return &(current->object.constants.val.i);
                break;
            case STRING:
                if (!strcmp(current->object.constants.val.s, (char *)value)) return current->object.constants.val.s;
                break;
            default:
                break;
        }
    }

    // If we made it here, we didn't find the constant.
    return 0;
}
