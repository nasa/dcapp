#include <cstring>
#include <ctime>
#include <sys/time.h>
#include "dcapp.h"

extern "C" void DisplayLogic(void)
{
    struct timeval tp;
    struct timezone tzp;
    static double deltax = 8, deltay = 8;

    gettimeofday(&tp, &tzp);

	strcpy(CURRENT_TIME, asctime(localtime((time_t *)(&tp.tv_sec))));
	CURRENT_TIME[strlen(CURRENT_TIME)-1] = 0;

    if (*POS_X < 10) deltax = 8;
    if (*POS_X > 1290) deltax = -8;
    *POS_X += deltax;

    if (*POS_Y < 10) deltay = 8;
    if (*POS_Y > 940) deltay = -8;
    *POS_Y += deltay;
}
