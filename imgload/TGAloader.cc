#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "imgload_internal.hh"

extern void setTGAImageData(ImageStruct *, unsigned int);

/*********************************************************************************
 *
 * This function is a Targa loader.
 *
 * Courtesy: Jeff Molofee (NeHe)  http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=24
 *
 *********************************************************************************/
unsigned int LoadTGA(const char *filename, ImageStruct *image)
{
    unsigned int bpp;
    unsigned char TGAheader[12] = {0,0,2,0,0,0,0,0,0,0,0,0}; // Uncompressed TGA Header
    unsigned char TGAcompare[12];                            // Used To Compare TGA Header
    unsigned char header[6];                                 // First 6 Useful Bytes From The Header
    unsigned int bytesPerPixel;                              // Holds Number Of Bytes Per Pixel Used In The TGA File
    unsigned int imageSize;                                  // Used To Store The Image Size When Setting Aside Ram
    unsigned int temp;                                       // Temporary Variable
    unsigned int i;

    FILE *file = fopen(filename, "r"); // Open The TGA File

    if (!file)
    {
        fprintf(stderr, "%s/%s: ERROR - (%s) does not exist\n", __FILE__, __FUNCTION__, filename);
        return 1;
    }

    if (fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||  // Are There 12 Bytes To Read?
        memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) !=0 ||                  // Does The Header Match What We Want?
        fread(header, 1, sizeof(header), file) != sizeof(header))                // If So Read Next 6 Header Bytes
    {
        fprintf(stderr, "%s/%s: ERROR - Something else failed\n", __FILE__, __FUNCTION__);
        fclose(file);
        return 2;
    }

    image->width  = header[1] * 256 + header[0]; // Determine The TGA Width (highbyte*256+lowbyte)
    image->height = header[3] * 256 + header[2]; // Determine The TGA Height (highbyte*256+lowbyte)

    // If width or height is invalid OR the TGA isn't 24- or 32-bit
    if (image->width <=0 || image->height <=0 || (header[4] != 24 && header[4] !=32 ))
    {
        fprintf(stderr, "%s/%s: ERROR - Bad bit count %d bits\n", __FILE__, __FUNCTION__, (int)header[0]);
        fclose(file);
        return 3;
    }

    bpp = header[4];                                         // Grab The TGA's Bits Per Pixel (24 or 32)
    bytesPerPixel = bpp/8;                                   // Divide By 8 To Get The Bytes Per Pixel
    imageSize = image->width*image->height*bytesPerPixel;    // Calculate The Memory Required For The TGA Data

    image->data = (unsigned char *)malloc(imageSize);        // Reserve Memory To Hold The TGA Data

    // If the storage memory doesn't exist or its size doesn't match the memory reserved
    if (!(image->data) || fread(image->data, 1, imageSize, file) != imageSize)
    {
        if (image->data) free(image->data);
        fprintf(stderr, "%s/%s: ERROR - Image data not loaded\n", __FILE__, __FUNCTION__);
        fclose(file);
        return 4;
    }

    for (i = 0; i < imageSize; i += bytesPerPixel) // Loop Through The Image Data
    {                                              // Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
        temp = image->data[i];                     // Temporarily Store The Value At Image Data 'i'
        image->data[i] = image->data[i + 2];       // Set The 1st Byte To The Value Of The 3rd Byte
        image->data[i + 2] = temp;                 // Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
    }

    fclose(file);
    setTGAImageData(image, bpp);
    return 0;
}
