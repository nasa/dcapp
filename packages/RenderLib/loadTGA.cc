#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "basicutils/msg.hh"
#include "texturelib.hh"

/*********************************************************************************
 * Create ImageStruct data from the contents of an uncompressed TARGA file
 *********************************************************************************/
int tdTexture::loadTGA(void)
{
    unsigned char expected[12] = {0,0,2,0,0,0,0,0,0,0,0,0}; // Expected header information
    unsigned char actual[12];                               // Actual header information
    unsigned char header[6];                                // First 6 useful bytes from the header
    unsigned int bitsPerPixel;                              // Bits per pixel
    unsigned int bytesPerPixel;                             // Bytes per pixel
    unsigned int imageSize;                                 // Size of the image data
    unsigned int temp;
    unsigned int i;

    FILE *file = fopen(this->filename.c_str(), "r");

    if (!file)
    {
        warning_msg("The file " << this->filename << " does not exist");
        return 1;
    }

    // Confirm that there are 12 bytes to read
    if (fread(actual, 1, sizeof(actual), file) != sizeof(actual))
    {
        warning_msg("Error reading header in " << this->filename);
        fclose(file);
        return 2;
    }

    // Confirm that the header contains the expected information
    if (memcmp(expected, actual, sizeof(expected)))
    {
        warning_msg("Error reading header in " << this->filename);
        fclose(file);
        return 3;
    }

    // Read the next 6 bytes in the header
    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        warning_msg("Error reading header in " << this->filename);
        fclose(file);
        return 4;
    }

    this->width  = header[1] * 256 + header[0];
    this->height = header[3] * 256 + header[2];
    bitsPerPixel = (unsigned int)header[4];

    // If width or height is invalid OR the TGA isn't 24- or 32-bit
    if (this->width <=0 || this->height <=0 || (bitsPerPixel != 24 && bitsPerPixel !=32))
    {
        warning_msg("Bad header in " << this->filename << ": bits-per-pixel=" << bitsPerPixel << ", width=" << this->width << ", height=" << this->height);
        fclose(file);
        return 5;
    }

    bytesPerPixel = bitsPerPixel/8;
    imageSize = this->width * this->height * bytesPerPixel;
    this->data = (unsigned char *)malloc(imageSize);

    // If the storage memory doesn't exist or its size doesn't match the memory reserved
    if (!(this->data) || fread(this->data, 1, imageSize, file) != imageSize)
    {
        if (this->data) free(this->data);
        warning_msg("Image data not loaded for file " << this->filename);
        fclose(file);
        return 6;
    }

    // Loop through the image data and swap 1st and 3rd bytes (red and blue)
    for (i = 0; i < imageSize; i += bytesPerPixel)
    {
        temp = this->data[i];
        this->data[i] = this->data[i + 2];
        this->data[i + 2] = temp;
    }

    fclose(file);

    if (bitsPerPixel == 24) this->pixelspec = PixelRGB;
    else this->pixelspec = PixelRGBA;

    return 0;
}
