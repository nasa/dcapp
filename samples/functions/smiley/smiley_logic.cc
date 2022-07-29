#define _DCAPP_EXTERNALS_
#include "../logic/dcapp.h"

// simple example scaling linewidth proportial to width and height
extern "C" void computeSmiley(void) {
    *_SMILEY_LINEWIDTH = (*_SMILEY_WIDTH + *_SMILEY_HEIGHT) / 2 / 100;
}
