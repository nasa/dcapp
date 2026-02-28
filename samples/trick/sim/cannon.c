/*****************************************************************************
PURPOSE: (Cannonball simulation)
*****************************************************************************/
#include <stdio.h>
#include <math.h>
#include "cannon.h"
#include "trick/exec_proto.h"

/* Time scale: sim runs 12x slower than physics time */
#define TIME_SCALE (1.0 / 12.0)

int cannon_init(CANNON *C) {
    double init_speed = 50.0;       /* m/s */
    double init_angle = M_PI / 6.0; /* 30 degrees */

    C->vel0[0]    = init_speed * cos(init_angle);
    C->vel0[1]    = init_speed * sin(init_angle);
    C->vel[0]     = C->vel0[0];
    C->vel[1]     = C->vel0[1];
    C->pos[0]     = 0.0;
    C->pos[1]     = 0.0;
    C->time       = 0.0;
    C->impact     = 0;
    C->impactTime = 0.0;

    return 0;
}

int cannon_update(CANNON *C) {
    double g = -9.81;
    double t;

    /* Don't update after impact */
    if (C->impact) {
        return 0;
    }

    /* Scale sim time to physics time */
    t       = exec_get_sim_time() * TIME_SCALE;
    C->time = t;

    /* Analytical solution */
    C->vel[0] = C->vel0[0];
    C->vel[1] = C->vel0[1] + g * t;
    C->pos[0] = C->vel0[0] * t;
    C->pos[1] = C->vel0[1] * t + 0.5 * g * t * t;

    /* Impact detection */
    if (C->pos[1] < 0.0) {
        C->impactTime = -2.0 * C->vel0[1] / g;
        C->pos[0]     = C->vel0[0] * C->impactTime;
        C->pos[1]     = 0.0;
        C->vel[0]     = 0.0;
        C->vel[1]     = 0.0;
        C->impact     = 1;
        fprintf(stderr, "\n*** IMPACT: t=%.2fs x=%.1fm ***\n\n", C->impactTime, C->pos[0]);
    }

    return 0;
}
