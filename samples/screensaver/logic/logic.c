#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "dcapp.h"

void display_init(void) {
}

void display_draw(void) {
    struct timeval tp;
    struct timezone tzp;
    static double deltax = 8, deltay = 8;

    gettimeofday(&tp, &tzp);
    char *time_str = asctime(localtime((time_t *)(&tp.tv_sec)));
    
    // Copy to CURRENT_TIME, remove trailing newline
    strncpy(*CURRENT_TIME, time_str, 255);
    size_t len = strlen(*CURRENT_TIME);
    if (len > 0 && (*CURRENT_TIME)[len-1] == '\n') {
        (*CURRENT_TIME)[len-1] = '\0';
    }

    if (*POS_X < 10) deltax = 8;
    if (*POS_X > 1290) deltax = -8;
    *POS_X += deltax;

    if (*POS_Y < 10) deltay = 8;
    if (*POS_Y > 940) deltay = -8;
    *POS_Y += deltay;
}

void display_close(void) {
}
