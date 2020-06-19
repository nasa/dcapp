#include <time.h>

void hibernate(float interval)
{
    struct timespec mytime;
    mytime.tv_sec = time_t(interval/1); // time_t
    mytime.tv_nsec = long((interval - mytime.tv_sec) * 1000000000); // long
    nanosleep(&mytime, 0x0);
}
