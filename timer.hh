#ifndef _TIMER_HH_
#define _TIMER_HH_

#include <sys/time.h>

class Timer
{
    public:
        Timer()
        {
            gettimeofday(&(this->stored), 0x0);
        };
        ~Timer()
        {
        };
        void restart(void)
        {
            gettimeofday(&(this->stored), 0x0);
        };
        float getSeconds(void)
        {
            struct timeval now;
            gettimeofday(&now, 0x0);
            return ((float)(now.tv_sec - (this->stored).tv_sec) + (0.000001 * (float)(now.tv_usec - (this->stored).tv_usec)));
        };
    private:
        struct timeval stored;
};

#endif
