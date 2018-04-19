#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "basicutils/msg.hh"
#include "imgload_internal.hh"

/*********************************************************************************
 * Create ImageStruct data from the contents of an uncompressed TARGA file
 *********************************************************************************/
unsigned int LoadTGA(const char *filename, ImageStruct *image)
{
    unsigned char expected[12] = {0,0,2,0,0,0,0,0,0,0,0,0}; // Expected header information
    unsigned char actual[12];                               // Actual header information
    unsigned char header[6];                                // First 6 useful bytes from the header
    unsigned int bitsPerPixel;                              // Bits per pixel
    unsigned int bytesPerPixel;                             // Bytes per pixel
    unsigned int imageSize;                                 // Size of the image data
    unsigned int temp;
    unsigned int i;

    FILE *file = fopen(filename, "r");

    if (!file)
    {
        warning_msg("The file " << filename << " does not exist");
        return 1;
    }

    // Confirm that there are 12 bytes to read
    if (fread(actual, 1, sizeof(actual), file) != sizeof(actual))
    {
        warning_msg("Error reading header in " << filename);
        fclose(file);
        return 2;
    }

    // Confirm that the header contains the expected information
    if (memcmp(expected, actual, sizeof(expected)))
    {
        warning_msg("Error reading header in " << filename);
        fclose(file);
        return 3;
    }

    // Read the next 6 bytes in the header
    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        warning_msg("Error reading header in " << filename);
        fclose(file);
        return 4;
    }

    image->width  = header[1] * 256 + header[0];
    image->height = header[3] * 256 + header[2];
    bitsPerPixel = (unsigned int)header[4];

    // If width or height is invalid OR the TGA isn't 24- or 32-bit
    if (image->width <=0 || image->height <=0 || (bitsPerPixel != 24 && bitsPerPixel !=32 ))
    {
        warning_msg("Bad header in " << filename << ": bits-per-pixel=" << bitsPerPixel << ", width=" << image->width << ", height=" << image->height);
        fclose(file);
        return 5;
    }

    bytesPerPixel = bitsPerPixel/8;
    imageSize = image->width * image->height * bytesPerPixel;
    image->data = (unsigned char *)malloc(imageSize);

    // If the storage memory doesn't exist or its size doesn't match the memory reserved
    if (!(image->data) || fread(image->data, 1, imageSize, file) != imageSize)
    {
        if (image->data) free(image->data);
        warning_msg("Image data not loaded for file " << filename);
        fclose(file);
        return 6;
    }

    // Loop through the image data and swap 1st and 3rd bytes (red and blue)
    for (i = 0; i < imageSize; i += bytesPerPixel)
    {
        temp = image->data[i];
        image->data[i] = image->data[i + 2];
        image->data[i + 2] = temp;
    }

    fclose(file);

    if (bitsPerPixel == 24) image->pixelspec = PixelRGB;
    else image->pixelspec = PixelRGBA;

    return 0;
}
