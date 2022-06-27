#include "dcapp.h"

extern "C" void DisplayLogic(void)
{
    static double deltar1 = 8;
    if (*RADIUS1 <= 30) deltar1 = 8;
    if (*RADIUS1 >= 500) deltar1 = -8;
    *RADIUS1 += deltar1;

    static double deltar2 = 5;
    if (*RADIUS2 <= 5) deltar2 = 5;
    if (*RADIUS2 >= 300) deltar2 = -5;
    *RADIUS2 += deltar2;

    static double deltar3 = 6;
    if (*RADIUS3 <= 20) deltar3 = 6;
    if (*RADIUS3 >= 400) deltar3 = -6;
    *RADIUS3 += deltar3;

    static double deltar4 = 2;
    if (*RADIUS4 <= 50) deltar4 = 2;
    if (*RADIUS4 >= 80) deltar4 = -2;
    *RADIUS4 += deltar4;
}
