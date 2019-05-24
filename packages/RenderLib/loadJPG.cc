#include <cstdio>
#include <cstdlib>
#ifdef JPEG_ENABLED
#include <jpeglib.h>
#endif
#include "basicutils/msg.hh"
#include "texturelib.hh"

/*********************************************************************************
 * Create ImageStruct data from the contents of a JPEG file.
 *********************************************************************************/
#ifdef JPEG_ENABLED
int tdTexture::loadJPG(void)
{
    FILE *file = fopen(this->filename.c_str(), "r");
    struct jpeg_decompress_struct jinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *line;

    if (!file)
    {
        warning_msg("The file " << this->filename << " does not exist");
        return 1;
    }

    jinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jinfo);
    jpeg_stdio_src(&jinfo, file);
    jpeg_read_header(&jinfo, TRUE);

    this->width = jinfo.image_width;
    this->height = jinfo.image_height;
    this->data = (unsigned char *)malloc(3 * jinfo.image_width * jinfo.image_height);
    this->pixelspec = PixelRGB;

    jpeg_start_decompress(&jinfo);
    while (jinfo.output_scanline < jinfo.output_height)
    {
        line = this->data + (3 * jinfo.image_width * (jinfo.output_height - jinfo.output_scanline - 1));
        jpeg_read_scanlines(&jinfo, &line, 1);
    }
    jpeg_finish_decompress(&jinfo);
    jpeg_destroy_decompress(&jinfo);

    fclose(file);

    return 0;
}
#else
int tdTexture::loadJPG(void)
{
    warning_msg("Could not find libjpeg or libjpeg-turbo");
    return 1;
}
#endif
