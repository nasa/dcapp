#include <cstring>
#include <ctime>
#include <sys/time.h>
#include "dcapp.hh"
#include "blinker.hh"

static blink_handler bh;

extern "C" void DisplayInit(void)
{
    /* map defining the different blinkers, with:
        * key for accessing the variable
        * blink manager for setting the DCAPP variable, num iterations, and duration per interval (ms)
    */
    bh.b_map = {
        {"IMAGE", blinker(IMAGE_BLINK_STATE, 4, 1000) },
        {"CLOCK", blinker(CLOCK_BLINK_STATE, 30, 200) }
    };

    bh.startBlinker("IMAGE");
    bh.startBlinker("CLOCK");

}

extern "C" void DisplayLogic(void)
{
    struct timeval tp;
    struct timezone tzp;
    static double deltax = 8, deltay = 8;

    gettimeofday(&tp, &tzp);
    *CURRENT_TIME = asctime(localtime((time_t *)(&tp.tv_sec)));
    if (!CURRENT_TIME->empty()) CURRENT_TIME->pop_back();

    if (*POS_X < 10) deltax = 8;
    if (*POS_X > 1290) deltax = -8;
    *POS_X += deltax;

    if (*POS_Y < 10) deltay = 8;
    if (*POS_Y > 940) deltay = -8;
    *POS_Y += deltay;

    // run every refresh cycle, to determine which graphics need to blink
    bh.processAllBlinkers();

}
