#include "dcapp.hpp"

void DisplayPreInit() {
    return;
}

void DisplayInit() {
    return;
}

void DisplayDraw() {
    ROTATE += .1;
    if (ROTATE > 360) {
        ROTATE = 0;
    }
    printf("%f %f %f\n", POS_X, POS_Y, VEL_Y);

    G += .001;
    return;
}

void DisplayClose() {
    return;
}
