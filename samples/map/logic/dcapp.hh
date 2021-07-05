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

double *LATITUDE;
double *LONGITUDE;
double *ZOOM;
int *ZoomSelected;
double *ZoomSlider;

#ifdef __cplusplus
extern "C" void DisplayPreInit(void *(*get_pointer_arg)(const char *))
#else
void DisplayPreInit(void *(*get_pointer_arg)(const char *))
#endif
{
    get_pointer = get_pointer_arg;

    LATITUDE = (double *)get_pointer("LATITUDE");
    LONGITUDE = (double *)get_pointer("LONGITUDE");
    ZOOM = (double *)get_pointer("ZOOM");
    ZoomSelected = (int *)get_pointer("ZoomSelected");
    ZoomSlider = (double *)get_pointer("ZoomSlider");
}

#else

extern void *(*get_pointer)(const char *);

extern double *LATITUDE;
extern double *LONGITUDE;
extern double *ZOOM;
extern int *ZoomSelected;
extern double *ZoomSlider;

#endif
