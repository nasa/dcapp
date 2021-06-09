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

std::string *CURRENT_TIME;
double *POS_X;
double *POS_Y;
int *IMAGE_BLINK_STATE;
int *CLOCK_BLINK_STATE;

#ifdef __cplusplus
extern "C" void DisplayPreInit(void *(*get_pointer_arg)(const char *))
#else
void DisplayPreInit(void *(*get_pointer_arg)(const char *))
#endif
{
    get_pointer = get_pointer_arg;

    CURRENT_TIME = (std::string *)get_pointer("CURRENT_TIME");
    POS_X = (double *)get_pointer("POS_X");
    POS_Y = (double *)get_pointer("POS_Y");
    IMAGE_BLINK_STATE = (int *)get_pointer("IMAGE_BLINK_STATE");
    CLOCK_BLINK_STATE = (int *)get_pointer("CLOCK_BLINK_STATE");
}

#else

extern void *(*get_pointer)(const char *);

extern std::string *CURRENT_TIME;
extern double *POS_X;
extern double *POS_Y;
extern int *IMAGE_BLINK_STATE;
extern int *CLOCK_BLINK_STATE;

#endif
