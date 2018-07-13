#include <cstdio>
#include "dcapp.h"

#define BaseWidth 46.0
#define BaseHeight 50.0

extern "C" void DisplayLogic(void)
{
    *IMAGE_WIDTH = *ZOOM * BaseWidth;
    *IMAGE_HEIGHT = *ZOOM * BaseHeight;
    *IMAGE_X = 50.0 + ((*PAN) * (*ZOOM));
    *IMAGE_Y = 50.0 + ((*TILT) * (*ZOOM));
}
