#ifndef _TIMER_HH_
#define _TIMER_HH_

#include <sys/time.h>

typedef struct timeval Timer;

#define StartTimer(a) gettimeofday(a, 0x0)

float SecondsElapsed(Timer);

#endif
