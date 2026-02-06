/*************************************************************************
PURPOSE: (Stress test data and functions)
LIBRARY DEPENDENCIES: ((stress.c))
**************************************************************************/
#ifndef STRESS_H
#define STRESS_H

#define STRESS_VAR_COUNT 2000

typedef struct {
    double values[STRESS_VAR_COUNT]; /* -- test values */
    double time;                     /* s  sim time    */
    int    count;                    /* -- var count   */
} STRESS_DATA;

#ifdef __cplusplus
extern "C" {
#endif

int stress_init(STRESS_DATA *S);
int stress_update(STRESS_DATA *S);

#ifdef __cplusplus
}
#endif

#endif
