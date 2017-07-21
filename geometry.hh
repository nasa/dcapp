#ifndef _GEOMETRY_HH_
#define _GEOMETRY_HH_

typedef struct
{
    float refx;
    float refy;
    float delx;
    float dely;
    float width;
    float height;
    float left;
    float right;
    float bottom;
    float top;
    float center;
    float middle;
} Geometry;

extern Geometry GetGeometry(float *, float *, float *, float *, float, float, int, int);

#endif
