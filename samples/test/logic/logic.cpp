#include "dcapp.h"

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
    return;
}

void DisplayClose() {
    return;
}
