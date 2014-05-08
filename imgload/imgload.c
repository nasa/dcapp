#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "imgload_internal.h"

extern unsigned int LoadTGA(char *filename, ImageStruct *);
extern int loadBMPImage(char *filename, ImageStruct *);
extern int createTextureFromImage(ImageStruct *);

static char *FindExtension(char *);


int imgload(char *filename)
{
    char *extension;
    ImageStruct image;
    int texture;

    extension = FindExtension(filename);

    if (!extension)
    {
        fprintf(stderr, "No detectable filename extension for file %s\n", filename);
        return (-1);
    }
    else if (!strcasecmp(extension, "BMP")) /* use ReaderWriterBMP.c */
    {
        if (loadBMPImage(filename, &image) == -1)
        {
            fprintf(stderr, "loadBMPImage returned with error for file %s\n", filename);
            return (-1);
        }
    }
    else if (!strcasecmp(extension, "TGA")) /* use TGAloader.c */
    {
        if (LoadTGA(filename, &image))
        {
            fprintf(stderr, "LoadTGA returned with error for file %s\n", filename);
            return (-1);
        }
    }
    else
    {
        fprintf(stderr, "Unsupported extension for file %s: %s\n", filename, extension);
        return (-1);
    }

    texture = createTextureFromImage(&image);
    if (image.data) free(image.data);
    return texture;
}


char *FindExtension(char *filename)
{
    char *end;
    
    if (!filename)
    {
        fprintf(stderr, "%s %d: No filename specified\n", __FILE__, __LINE__);
        return NULL;
    }

    end = &(filename[strlen(filename)-1]);
    while (*end != '.' && end != filename) end--;

    if (end == filename)
    {
        fprintf(stderr, "%s %d: Couldn't find extension in filename '%s'\n", __FILE__, __LINE__, filename);
        return NULL;
    }

    return (end+1);
}
