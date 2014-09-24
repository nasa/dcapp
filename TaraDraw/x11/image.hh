#ifndef _IMAGE_HH_ 
#define _IMAGE_HH_

typedef struct
{
    float red;
    float green;
    float blue;
    float alpha;
} TDColorSpec;

typedef struct
{
    float width;
    float height;
    unsigned ncolors;
    TDColorSpec *color;
    unsigned long *xcolor;
    unsigned *pixel;
} TDImage;

#endif