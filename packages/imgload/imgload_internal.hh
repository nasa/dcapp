#ifndef _IMGLOAD_INTERNAL_HH_
#define _IMGLOAD_INTERNAL_HH_

#include <string>

typedef enum { PixelUnknown, PixelLuminance, PixelLuminanceAlpha, PixelRGB, PixelRGBA } PixelSpec;

typedef struct
{
    int width;
    int height;
    PixelSpec pixelspec;
    unsigned char *data;
} ImageStruct;

#endif
