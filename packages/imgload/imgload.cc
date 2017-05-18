#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include "utils/msg.hh"
#include "imgload_internal.hh"

extern unsigned int LoadTGA(const char *, ImageStruct *);
extern int LoadBMP(const char *, ImageStruct *);
extern unsigned int LoadJPG(const char *, ImageStruct *);
extern int createTextureFromImage(ImageStruct *);

static const char *FindExtension(const char *filename)
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
        error_msg("No detectable filename extension for file " << filename);
        return 0x0;
    }

    return (end+1);
}

int imgload(const char *filename)
{
    const char *extension;
    ImageStruct image;
    int texture;

    extension = FindExtension(filename);

    if (!extension)
    {
        return (-1);
    }
    else if (!strcasecmp(extension, "BMP"))
    {
        if (LoadBMP(filename, &image) == -1)
        {
            error_msg("LoadBMP returned with error for file " << filename);
            return (-1);
        }
    }
    else if (!strcasecmp(extension, "TGA"))
    {
        if (LoadTGA(filename, &image))
        {
            error_msg("LoadTGA returned with error for file " << filename);
            return (-1);
        }
    }
    else if (!strcasecmp(extension, "JPG") || !strcasecmp(extension, "JPEG"))
    {
        if (LoadJPG(filename, &image))
        {
            error_msg("LoadJPG returned with error for file " << filename);
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
