/*************************************************************************
PURPOSE: (Cannonball state and functions)
LIBRARY DEPENDENCIES: ((cannon.c))
**************************************************************************/
#ifndef CANNON_H
#define CANNON_H

typedef struct {
    double pos[2];      /* m    xy-position */
    double vel[2];      /* m/s  xy-velocity */
    double vel0[2];     /* m/s  initial velocity */
    double time;        /* s    sim time */
    int    impact;      /* --   impact flag */
    double impactTime;  /* s    time of impact */
} CANNON;

#ifdef __cplusplus
extern "C" {
#endif
    int cannon_init(CANNON*);
    int cannon_update(CANNON*);
#ifdef __cplusplus
}
#endif

#endif
