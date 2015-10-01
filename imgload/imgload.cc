#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "msg.hh"
#include "imgload_internal.hh"

extern unsigned int LoadTGA(const char *filename, ImageStruct *);
extern int loadBMPImage(const char *filename, ImageStruct *);
extern int createTextureFromImage(ImageStruct *);

static const char *FindExtension(const char *);


int imgload(const char *filename)
{
    const char *extension;
    ImageStruct image;
    int texture;

    extension = FindExtension(filename);

    if (!extension)
    {
        error_msg("No detectable filename extension for file " << filename);
        return (-1);
    }
    else if (!strcasecmp(extension, "BMP")) /* use ReaderWriterBMP.c */
    {
        if (loadBMPImage(filename, &image) == -1)
        {
            error_msg("loadBMPImage returned with error for file " << filename);
            return (-1);
        }
    }
    else if (!strcasecmp(extension, "TGA")) /* use TGAloader.c */
    {
        if (LoadTGA(filename, &image))
        {
            error_msg("LoadTGA returned with error for file " << filename);
            return (-1);
        }
    }
    else
    {
        error_msg("Unsupported extension for file " << filename << ": " << extension);
        return (-1);
    }

    texture = createTextureFromImage(&image);
    if (image.data) free(image.data);
    return texture;
}


const char *FindExtension(const char *filename)
{
    if (!filename)
    {
        error_msg("No filename specified");
        return 0x0;
    }

    const char *end = &(filename[strlen(filename)-1]);
    while (*end != '.' && end != filename) end--;

    if (end == filename)
    {
        error_msg("Couldn't find extension in filename " << filename);
        return 0x0;
    }

    return (end+1);
}
