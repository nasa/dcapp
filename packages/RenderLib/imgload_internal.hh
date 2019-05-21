#ifndef _IMGLOAD_INTERNAL_HH_
#define _IMGLOAD_INTERNAL_HH_

typedef enum { PixelUnknown, PixelLuminance, PixelLuminanceAlpha, PixelRGB, PixelRGBA } PixelSpec;

typedef struct
{
    int width;
    int height;
    PixelSpec pixelspec;
    unsigned char *data;
} ImageStruct;

#endif
