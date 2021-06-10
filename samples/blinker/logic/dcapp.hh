// ********************************************* //
// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //
// ********************************************* //

#define DCAPP_VERSION_1_0

#define DCAPP_MAJOR_VERSION 2
#define DCAPP_MINOR_VERSION 0

#include <string>

#ifndef _DCAPP_EXTERNALS_
#define _DCAPP_EXTERNALS_

void *(*get_pointer)(const char *);

int *CIRCLE_BLINK_STATE;
int *CIRCLE_START_BLINK;
int *CIRCLE_STOP_BLINK;

#ifdef __cplusplus
extern "C" void DisplayPreInit(void *(*get_pointer_arg)(const char *))
#else
void DisplayPreInit(void *(*get_pointer_arg)(const char *))
#endif
{
    get_pointer = get_pointer_arg;

    CIRCLE_BLINK_STATE = (int *)get_pointer("CIRCLE_BLINK_STATE");
    CIRCLE_START_BLINK = (int *)get_pointer("CIRCLE_START_BLINK");
    CIRCLE_STOP_BLINK = (int *)get_pointer("CIRCLE_STOP_BLINK");
}

#else

extern void *(*get_pointer)(const char *);

extern int *CIRCLE_BLINK_STATE;
extern int *CIRCLE_START_BLINK;
extern int *CIRCLE_STOP_BLINK;

#endif
