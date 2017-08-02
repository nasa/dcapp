#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "image.hh"

#define BytesToInt4(a) ((16777216 * a[3]) + (65536 * a[2]) + (256 * a[1]) + a[0])
#define BytesToInt2(a) ((256 * a[1]) + a[0])

typedef struct
{
    unsigned char identifier[2];
    unsigned char filesize[4];
    unsigned char reserved[4];
    unsigned char offset[4];
    unsigned char headersize[4];
    unsigned char imagewidth[4];
    unsigned char imageheight[4];
    unsigned char ncolplanes[2];
    unsigned char bitsperpixel[2];
    unsigned char compression[4];
    unsigned char imagesize[4];
    unsigned char hrez[4];
    unsigned char vrez[4];
    unsigned char ncols[4];
    unsigned char nimportant[4];
} BmpHeader;

typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} BmpPixel;

typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char unused;
} BmpColorMap;

typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} TmpColorTable;

TDImage *tdLoadBmpImage(const char *filespec)
{
    BmpHeader header;
    int imagewidth, imageheight, ncols;
    int npixels, index, i, j, k, pad, foundmatch, colcount = 0;
    char dummy[4];
    TmpColorTable *ct;
    TDImage *image;

    FILE *infile = fopen(filespec, "r");
    if (infile == 0)
    {
        char *tddata = getenv("TARADRAW_DATA");
        if (tddata == 0) return 0;
        char *fullpath = (char *)malloc(strlen(tddata) + strlen(filespec) + 2);
        if (fullpath == 0) return 0;
        sprintf(fullpath, "%s/%s", tddata, filespec);
        infile = fopen(fullpath, "r");
        free(fullpath);
        if (infile == 0) return 0;
    }
    fread(&header, sizeof(BmpHeader), 1, infile);

    imagewidth = BytesToInt4(header.imagewidth);
    imageheight = BytesToInt4(header.imageheight);
    ncols = BytesToInt4(header.ncols);

    npixels = imagewidth * imageheight;

    image = (TDImage *)malloc(sizeof(TDImage));
    if (image == 0) return 0;
    image->pixel = (unsigned int *)malloc(npixels * sizeof(unsigned));
    if (image->pixel == 0) return 0;

    if (ncols)
    {
        unsigned *bmpindex;
        unsigned char *pixmap;
        BmpColorMap *bcm;

        if (imagewidth%4) pad = 4 - (imagewidth%4); else pad = 0;

        bmpindex = (unsigned int *)malloc(ncols * sizeof(unsigned));
        if (bmpindex == 0) return 0;
        bcm = (BmpColorMap *)malloc(ncols * sizeof(BmpColorMap));
        if (bcm == 0) return 0;
        fread(bcm, sizeof(BmpColorMap), ncols, infile);

        ct = (TmpColorTable *)calloc(ncols, sizeof(TmpColorTable));
        if (ct == 0) return 0;
        for (i=0; i<ncols; i++)
        {
            foundmatch = 0;
            for (j=0; j<colcount && !foundmatch; j++)
            {
                if (bcm[i].red == ct[j].red &&
                    bcm[i].green == ct[j].green &&
                    bcm[i].blue == ct[j].blue)
                {
                    foundmatch = 1;
                    bmpindex[i] = j;
                }
            }
            if (!foundmatch)
            {
                ct[colcount].red = bcm[i].red;
                ct[colcount].green = bcm[i].green;
                ct[colcount].blue = bcm[i].blue;
                bmpindex[i] = colcount;
                colcount++;
            }
        }

        free(bcm);

        image->ncolors = colcount;
        image->color = (TDColorSpec *)calloc(colcount, sizeof(TDColorSpec));
        if (image->color == 0) return 0;
        for (i=0; i<colcount; i++)
        {
            image->color[i].red = (float)(ct[i].red)/256;
            image->color[i].green = (float)(ct[i].green)/256;
            image->color[i].blue = (float)(ct[i].blue)/256;
        }

        free(ct);

        pixmap = (unsigned char *)malloc(imagewidth * sizeof(unsigned char));
        if (pixmap == 0) return 0;

        for (i=0; i<imageheight; i++)
        {
            fread(pixmap, sizeof(unsigned char), imagewidth, infile);
            if (pad) fread(dummy, sizeof(char), pad, infile);
            for (j=0; j<imagewidth; j++)
            {
                index = (imagewidth*i)+j;
                image->pixel[index] = bmpindex[pixmap[j]];
            }
        }

        fclose(infile);
        free(pixmap);
        free(bmpindex);
    }
    else
    {
        BmpPixel *bmpimg = (BmpPixel *)malloc(imagewidth * sizeof(BmpPixel));
        if (bmpimg == 0) return 0;

        pad = imagewidth%4;

        ct = (TmpColorTable *)calloc(npixels, sizeof(TmpColorTable));
        if (ct == 0) return 0;
        for (i=0; i<imageheight; i++)
        {
            fread(bmpimg, sizeof(BmpPixel), imagewidth, infile);
            if (pad) fread(dummy, sizeof(char), pad, infile);
            for (j=0; j<imagewidth; j++)
            {
                index = (imagewidth*i)+j;
                foundmatch = 0;
                for (k=0; k<colcount && !foundmatch; k++)
                {
                    if (bmpimg[j].red == ct[k].red &&
                        bmpimg[j].green == ct[k].green &&
                        bmpimg[j].blue == ct[k].blue)
                    {
                        image->pixel[index] = k;
                        foundmatch = 1;
                    }
                }
                if (!foundmatch)
                {
                    ct[colcount].red = bmpimg[j].red;
                    ct[colcount].green = bmpimg[j].green;
                    ct[colcount].blue = bmpimg[j].blue;
                    image->pixel[index] = colcount;
                    colcount++;
                }
            }
        }

        free(bmpimg);
        fclose(infile);

        image->ncolors = colcount;
        image->color = (TDColorSpec *)calloc(colcount, sizeof(TDColorSpec));
        if (image->color == 0) return 0;
        for (i=0; i<colcount; i++)
        {
            image->color[i].red = (float)(ct[i].red)/256;
            image->color[i].green = (float)(ct[i].green)/256;
            image->color[i].blue = (float)(ct[i].blue)/256;
        }

        free(ct);
    }


    image->width = (float)imagewidth;
    image->height = (float)imageheight;

    return image;
}
