#ifndef _DC_HH_
#define _DC_HH_

#include "fontlib.hh"

typedef flFont *dcFont;
typedef unsigned int dcTexture;

typedef enum
{
    AlignBottomLeft,
    AlignBottomCenter,
    AlignBottomRight,
    AlignMiddleLeft,
    AlignMiddleCenter,
    AlignMiddleRight,
    AlignTopLeft,
    AlignTopCenter,
    AlignTopRight
} dcAlign;

typedef struct
{
    float x;
    float y;
} dcPosition;

typedef struct
{
    float w;
    float h;
} dcSize;

typedef struct
{
    float left;
    float right;
    float bottom;
    float top;
    float refx;
    float refy;
    float deltax;
    float deltay;
} dcBounds;

typedef struct
{
    float r;
    float g;
    float b;
    float a;
} dcColor;

#endif
