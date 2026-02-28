/*****************************************************************************
PURPOSE: (Stress test simulation - 2000 variables)
*****************************************************************************/
#include "stress.h"
#include <math.h>
#include "trick/exec_proto.h"

int stress_init(STRESS_DATA *S) {
    S->count = STRESS_VAR_COUNT;
    S->time  = 0.0;
    for (int i = 0; i < STRESS_VAR_COUNT; i++) {
        S->values[i] = (double)i;
    }
    return 0;
}

int stress_update(STRESS_DATA *S) {
    double t = exec_get_sim_time();
    S->time  = t;

    /* each variable gets a unique sin wave so we can verify correct mapping */
    for (int i = 0; i < STRESS_VAR_COUNT; i++) {
        double freq  = 0.1 + (double)i * 0.001;
        double phase = (double)i * 0.1;
        S->values[i] = sin(freq * t + phase) * (100.0 + (double)i);
    }
    return 0;
}
