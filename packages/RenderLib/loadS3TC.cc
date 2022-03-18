#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "basicutils/msg.hh"
#include "texturelib.hh"

/**************************************************************************************
 * Create ImageStruct data from the contents of a GL_COMPRESSED_SRGB_S3TC_DXT1_EXT file
 **************************************************************************************/
int tdTexture::loadS3TC(void)
{
    FILE *file = fopen(this->filename.c_str(), "r");

    if (!file)
    {
        warning_msg("The file " << this->filename << " does not exist");
        return 1;
    }

    this->width  = 17625;
    this->height = 17625;
    this->data = (unsigned char *)malloc(155373192);

    fread(this->data, 1, 155373192, file);
    fclose(file);

    this->pixelspec = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

    return 0;
}
