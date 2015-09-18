/*
 * SGI rgb file reader borrowed from gltk library
 */

//#include "togl.h"               /* added by GG to include windows.h */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imgload_internal.hh"

extern void setRGBImageData (ImageStruct *, unsigned short, unsigned short, unsigned short);

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

static void tkQuit(void)
{
    exit(0);
}

/******************************************************************************/

typedef struct _rawImageRec
{
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    unsigned long min, max;
    unsigned long wasteBytes;
    char    name[80];
    unsigned long colorMap;
    FILE   *file;
    unsigned char *tmp, *tmpR, *tmpG, *tmpB, *tmpA;
    unsigned long rleEnd;
    unsigned int *rowStart;
    int  *rowSize;
} rawImageRec;

/******************************************************************************/

static void ConvertShort(unsigned short *array, long length)
{
    unsigned long b1, b2;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--)
    {
        b1 = *ptr++;
        b2 = *ptr++;
        *array++ = (b1 << 8) | (b2);
    }
}

static void ConvertLong(unsigned int *array, long length)
{
    unsigned long b1, b2, b3, b4;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--)
    {
        b1 = *ptr++;
        b2 = *ptr++;
        b3 = *ptr++;
        b4 = *ptr++;
        *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
    }
}

static rawImageRec *RawImageOpen(char *fileName)
{
    union
    {
        int testWord;
        char testByte[4];
    } endianTest;
    rawImageRec *raw;
    int swapFlag;
    int x;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) swapFlag = 1;
    else swapFlag = 0;

    raw = (rawImageRec *)malloc(sizeof(rawImageRec));
    if (!raw)
    {
        fprintf(stderr, "Out of memory!\n");
        tkQuit();
    }
    if (!(raw->file = fopen(fileName, "rb")))
    {
        perror(fileName);
        tkQuit();
    }

    fread(raw, 1, 12, raw->file);

    if (swapFlag) ConvertShort(&raw->imagic, 6);

    raw->tmp = (unsigned char *) malloc(raw->sizeX * 256);
    raw->tmpR = (unsigned char *) malloc(raw->sizeX * 256);
    raw->tmpG = (unsigned char *) malloc(raw->sizeX * 256);
    raw->tmpB = (unsigned char *) malloc(raw->sizeX * 256);
    raw->tmpA = (unsigned char *) malloc(raw->sizeX * 256);
    if (!(raw->tmp) || !(raw->tmpR) || !(raw->tmpG) || !(raw->tmpB) || !(raw->tmpA))
    {
        fprintf(stderr, "Out of memory!\n");
        tkQuit();
    }

    if ((raw->type & 0xFF00) == 0x0100)
    {
        x = raw->sizeY * raw->sizeZ * sizeof (unsigned int);
        raw->rowStart = (unsigned int *) malloc(x);
        raw->rowSize = (int *) malloc(x);
        if (!(raw->rowStart) || !(raw->rowSize))
        {
            fprintf(stderr, "Out of memory!\n");
            tkQuit();
        }
        raw->rleEnd = 512 + (2 * x);
        fseek(raw->file, 512, SEEK_SET);
        fread(raw->rowStart, 1, x, raw->file);
        fread(raw->rowSize, 1, x, raw->file);
        if (swapFlag)
        {
            ConvertLong(raw->rowStart, x / sizeof (unsigned int));
            ConvertLong((unsigned int *) raw->rowSize, x / sizeof (int));
        }
    }
    return raw;
}

static void RawImageClose(rawImageRec *raw)
{
    fclose(raw->file);
    free(raw->tmp);
    free(raw->tmpR);
    free(raw->tmpG);
    free(raw->tmpB);
    free(raw->tmpA);
    free(raw);
}

static void RawImageGetRow(rawImageRec *raw, unsigned char *buf, int y, int z)
{
    unsigned char *iPtr, *oPtr, pixel;
    int     count;

    if ((raw->type & 0xFF00) == 0x0100)
    {
        fseek(raw->file, raw->rowStart[y + z * raw->sizeY], SEEK_SET);
        fread(raw->tmp, 1, (unsigned int) raw->rowSize[y + z * raw->sizeY], raw->file);

        iPtr = raw->tmp;
        oPtr = buf;
        while (1)
        {
            pixel = *iPtr++;
            count = (int) (pixel & 0x7F);
            if (!count) return;
            if (pixel & 0x80)
            {
                while (count--) *oPtr++ = *iPtr++;
            }
            else
            {
                pixel = *iPtr++;
                while (count--) *oPtr++ = pixel;
            }
        }
    }
    else
    {
        fseek(raw->file, 512 + (y * raw->sizeX) + (z * raw->sizeX * raw->sizeY), SEEK_SET);
        fread(buf, 1, raw->sizeX, raw->file);
    }
}

static void RawImageGetData(rawImageRec *raw, ImageStruct *final)
{
    unsigned char *ptr;
    int i, j;

    final->data = (unsigned char *) malloc((raw->sizeX + 1) * (raw->sizeY + 1) * 4);
    if (!(final->data))
    {
        fprintf(stderr, "Out of memory!\n");
        tkQuit();
    }

    ptr = final->data;
    for (i = 0; i < (int) (raw->sizeY); i++)
    {
        RawImageGetRow(raw, raw->tmpR, i, 0);
        RawImageGetRow(raw, raw->tmpG, i, 1);
        RawImageGetRow(raw, raw->tmpB, i, 2);
        if (raw->sizeZ == 4)
        {
            /* 4 components */
            RawImageGetRow(raw, raw->tmpA, i, 3);
            for (j = 0; j < (int) (raw->sizeX); j++)
            {
                *ptr++ = *(raw->tmpR + j);
                *ptr++ = *(raw->tmpG + j);
                *ptr++ = *(raw->tmpB + j);
                *ptr++ = *(raw->tmpA + j);
            }
        }
        else
        {
            /* 3 components */
            for (j = 0; j < (int) (raw->sizeX); j++)
            {
                *ptr++ = *(raw->tmpR + j);
                *ptr++ = *(raw->tmpG + j);
                *ptr++ = *(raw->tmpB + j);
            }
        }
    }
}

unsigned int LoadRGBTexture(char *fileName, ImageStruct *final)
{
    rawImageRec *raw = RawImageOpen(fileName);
    setRGBImageData (final, raw->sizeX,raw->sizeY,raw->sizeZ );
    RawImageGetData(raw, final);
    RawImageClose(raw);
    return 0;
}
