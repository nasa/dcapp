// ********************************************* //
// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //
// ********************************************* //

#ifndef _DCAPP_EXTERNALS_
#define _DCAPP_EXTERNALS_

char *CURRENT_TIME;
char *MY_STR;
float *POS_X;
float *POS_Y;

#ifdef __cplusplus
extern "C" void DisplayPreInit(void *(*get_pointer)(const char *))
#else
void DisplayPreInit(void *(*get_pointer)(const char *))
#endif
{
    CURRENT_TIME = (char *)get_pointer("CURRENT_TIME");
    MY_STR = (char *)get_pointer("MY_STR");
    POS_X = (float *)get_pointer("POS_X");
    POS_Y = (float *)get_pointer("POS_Y");
}

#else

extern char *CURRENT_TIME;
extern char *MY_STR;
extern float *POS_X;
extern float *POS_Y;

#endif
