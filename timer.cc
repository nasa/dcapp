#include <sys/time.h>

typedef struct timeval Timer;

float SecondsElapsed(Timer mytimer)
{
    struct timeval now;
    gettimeofday(&now, 0x0);
    return ((float)(now.tv_sec - mytimer.tv_sec) + (0.000001 * (float)(now.tv_usec - mytimer.tv_usec)));
}
