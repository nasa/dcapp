#include <cstdio>
#include <cstdlib>
#ifdef JPEG_ENABLED
#include <jpeglib.h>
#endif
#include "imgload_internal.hh"

extern void setRGBImageData(ImageStruct *, unsigned short, unsigned short, unsigned short);

unsigned int LoadJPG(const char *filename, ImageStruct *image)
{
#ifdef JPEG_ENABLED
    FILE *file = fopen(filename, "r");
    struct jpeg_decompress_struct jinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *line;

    if (!file)
    {
        fprintf(stderr, "%s/%s: ERROR - (%s) does not exist\n", __FILE__, __FUNCTION__, filename);
        return 1;
    }

    jinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jinfo);
    jpeg_stdio_src(&jinfo, file);
    jpeg_read_header(&jinfo, 1);

    image->data = (unsigned char *)malloc(3 * jinfo.image_width * jinfo.image_height);

    jpeg_start_decompress(&jinfo);
    while (jinfo.output_scanline < jinfo.output_height)
    {
        line = image->data + (3 * jinfo.image_width * (jinfo.output_height - jinfo.output_scanline - 1));
        jpeg_read_scanlines(&jinfo, &line, 1);
    }
    jpeg_finish_decompress(&jinfo);
    jpeg_destroy_decompress(&jinfo);

    image->width = jinfo.image_width;
    image->height = jinfo.image_height;
    setRGBImageData(image, image->width, image->height, 3);

    return 0;
#else
    fprintf(stderr, "%s/%s: WARNING - Could not find libjpeg or libjpeg-turbo\n", __FILE__, __FUNCTION__);
    return 1;
#endif
}
