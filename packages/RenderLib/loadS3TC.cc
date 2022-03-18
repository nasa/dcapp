#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "basicutils/msg.hh"
#include "texturelib.hh"

/**************************************************************************************
 * Create ImageStruct data from the contents of a GL_COMPRESSED_SRGB_S3TC_DXT1_EXT file
 **************************************************************************************/

typedef struct {
    uint32_t format;
    int32_t level;
    uint32_t width;
    uint32_t height;
    uint32_t size;
    uint8_t padding[44];
} S3TCheader;

int tdTexture::loadS3TC(void)
{
    FILE *file = fopen(this->filename.c_str(), "r");

    if (!file)
    {
        warning_msg("The file " << this->filename << " does not exist");
        return 1;
    }

    S3TCheader header;
    fread(&header, 1, sizeof(S3TCheader), file);

    this->width  = (int)(header.width);
    this->height = (int)(header.height);
    this->data = (unsigned char *)malloc(header.size);

    fread(this->data, 1, header.size, file);
    fclose(file);

    this->pixelspec = header.format;

    return 0;
}
