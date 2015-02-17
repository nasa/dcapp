/****************************************************************************
 *
 * Follows is code written by GWM and translated to fit with the OSG Ethos.
 *
 *
 * Ported into the OSG as a plugin, Geoff Michel October 2001.
 * For patches, bugs and new features
 * please send them direct to the OSG dev team.
 *
 **********************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "imgload_internal.hh"

// BMP format bits - at start of file is 512 bytes of pure garbage
enum ftype { MB=19778 }; // magic number identifies a bmp file; actually chars 'B''M'
// allowed ftypes are 'BM'  for windoze;  OS2 allows:
//'BA' - Bitmap Array
//'CI' - Color Icon
//'CP' - Color Pointer (mouse cursor)
//'IC' - Icon
//'PT' - Pointer (mouse cursor)

typedef struct
{
    short FileType;             // always MB
    unsigned short siz[2];      // a dword for whole file size - make unsigned Feb 2002
    short Reserved1, Reserved2; // reserved for future purposes
    unsigned short offset[2];   // offset to image in bytes
} bmpheader;

typedef struct
{
    int width;        // width of the image in pixels
    int height;       // height of the image in pixels
    short planes;     // :word: number of planes (always 1)
    short Colorbits;  // word: number of bits used to describe color in each pixel
    int compression;  // compression used
    int ImageSize;    // image size in bytes
    int XpixPerMeter; // pixels per meter in X
    int YpixPerMeter; // pixels per meter in Y
    int ColorUsed;    // number of colors used
    int Important;    // number of "important" colors
} BMPInfo;

extern void setBMPImageData(ImageStruct *, ncol);

/* byte order workarounds *sigh* */
void swapbyte_L(int *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[3];
    vv[3]=tmp;
    tmp=vv[1];
    vv[1]=vv[2];
    vv[2]=tmp;
}
void swapbyte_UL(unsigned int *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[3];
    vv[3]=tmp;
    tmp=vv[1];
    vv[1]=vv[2];
    vv[2]=tmp;
}
void swapbyte_F(float *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[3];
    vv[3]=tmp;
    tmp=vv[1];
    vv[1]=vv[2];
    vv[2]=tmp;
}
void swapbyte_US(unsigned short *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[1];
    vv[1]=tmp;
}
void swapbyte_S(short *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[1];
    vv[1]=tmp;
}

// reads filename, and returns the buffer
// bmp is very very simple format
// even Master Gates could have invented it.
// It is extremely expensive on disk space - every RGB pixel uses 3 bytes plus a header!
// BMP - sponsored by Seagate.
int loadBMPImage(const char *filename, ImageStruct *image)
{
    unsigned char *buffer = NULL; // returned to sender & as read from the disk
    int filelen;
    int ncolours;
    int ncomp = 0;
    int swap = 0; // dont need to swap bytes
    bmpheader hd; // actual size of the bitmap header; 12=os2; 40 = normal; 64=os2.1
    BMPInfo inf;

    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "%s/%s: ERROR - (%s) does not exist\n", __FILE__, __FUNCTION__, filename);
        image->data = NULL;
        return (-1);
    }

    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp); // determine file size so we can fill it in later if FileSize == 0
    fseek(fp, 0, SEEK_SET);

    fread((char *)&hd, sizeof(bmpheader), 1, fp);
    if (hd.FileType != MB)
    {
        swapbyte_S(&(hd.FileType));
        swap = 1;
    }
    if (hd.FileType == MB)
    {
        int infsize;    //size of BMPinfo in bytes
        unsigned char *cols = NULL; // dynamic colour palette
        unsigned char *imbuff; // returned to sender & as read from the disk
        fread((char *)&infsize, sizeof(int), 1, fp); // insert inside 'the file is bmp' clause
        if (swap) swapbyte_L(&infsize);
        unsigned char *hdr = (unsigned char *)malloc(infsize); // to hold the new header
        fread((char *)hdr, 1,infsize-sizeof(int), fp);
        int hsiz = sizeof(inf); // minimum of structure size &
        if (infsize <= hsiz) hsiz = infsize;
        memcpy(&inf, hdr, hsiz/*-sizeof(int)*/); // copy only the bytes I can cope with
        free(hdr);

        if (swap)
        { // inverse the field of the header which need swapping
            swapbyte_US(&hd.siz[0]);
            swapbyte_US(&hd.siz[1]);
            swapbyte_S(&inf.Colorbits);
            swapbyte_L(&inf.width);
            swapbyte_L(&inf.height);
            swapbyte_L(&inf.ImageSize);
        }
        if (infsize == 12)
        { // os2, protect us from our friends ? || infsize==64
            int wd = inf.width&0xffff; // shorts replace ints
            int ht = inf.width>>16;
            int npln = inf.height&0xffff; // number of planes
            int cbits = inf.height>>16;
            inf.width = wd;
            inf.height = ht;
            inf.planes = npln;
            inf.Colorbits = cbits;
            inf.ColorUsed = (int)pow(2.0, (double)inf.Colorbits); // infer the colours
        }

        // previous size calculation, see new calcs below.

        // order of size calculation swapped, by Christo Zietsman to fix size bug.
        int size = hd.siz[1]*65536 + hd.siz[0];

        // handle size == 0 in uncompressed 24-bit BMPs -Eric Hammil
        if (size == 0) size = filelen;

        int ncpal = 4; // default number of colours per palette entry
        size -= sizeof(bmpheader) + infsize;
        if (inf.ImageSize<size) inf.ImageSize=size;
        imbuff = (unsigned char *)malloc(inf.ImageSize); // read from disk

        //The following line reads in the actual data from the image
        //tmp should eqaul inf.ImageSize.  If not there is a problem
        int tmp = fread((char *)imbuff, sizeof(unsigned char),inf.ImageSize, fp);
        if(tmp != inf.ImageSize)
        {
            fprintf(stderr, "%s/%s: ERROR - (%s) problem retrieving image size\n", __FILE__, __FUNCTION__, filename);
            image->data = NULL;
            return (-1);
        }
        ncolours = inf.Colorbits/8;
        switch (ncolours)
        {
        case 1:
            ncomp = NCOL_BW; // actually this is a 256 colour, paletted image
            inf.Colorbits = 8; // so this is how many bits there are per index
            inf.ColorUsed = 256; // and number of colours used
            cols=imbuff; // colour palette address - uses 4 bytes/colour
            break;
        case 2:
            ncomp = NCOL_IA;
            break;
        case 3:
            ncomp = NCOL_RGB;
            break;
        case 4:
            ncomp = NCOL_RGBA;
            break;
        default:
            cols = imbuff; // colour palette address - uses 4 bytes/colour
            if (infsize == 12 || infsize == 64) ncpal = 3; // OS2 - uses 3 colours per palette entry
            else ncpal = 4; // Windoze uses 4!
        }

        if (ncomp > 0) buffer = (unsigned char *)malloc((ncomp==NCOL_BW?3:ncomp)*inf.width*inf.height); // to be returned
        else buffer = (unsigned char *)malloc(3*inf.width*inf.height); // default full colour to be returned

        unsigned int off = 0;
        unsigned int rowbytes = ncomp*sizeof(unsigned char)*inf.width;
        unsigned int doff = (rowbytes)/4;
        if ((rowbytes%4)) doff++; // round up if needed
        doff *= 4; // to find dword alignment
        int j;
        for(j = 0; j < inf.height; j++)
        {
            if (ncomp>NCOL_BW) memcpy(buffer+j*rowbytes, imbuff+off, rowbytes); // pack bytes closely
            else
            { // find from the palette..
                unsigned char *imptr = imbuff+inf.ColorUsed*ncpal; // add size of the palette- start of image
                int npixperbyte = 8/inf.Colorbits; // no of pixels per byte
                int ii;
                for(ii = 0; ii < inf.width/npixperbyte; ii++)
                {
                    unsigned char mask = 0x00; // masked with index to extract colorbits bits
                    unsigned char byte = imptr[(j*inf.width/npixperbyte)+ii];
                    int jj;
                    for (jj=0; jj<inf.Colorbits; jj++) mask |= (0x80>>jj); // fill N High end bits
                    for (jj=0; jj<npixperbyte; jj++)
                    {
                        int colidx = (byte&mask)>>((npixperbyte-1-jj)*inf.Colorbits);
                        buffer[3*(j*inf.width+ii*npixperbyte+jj)+0] = cols[ncpal*colidx+2];
                        buffer[3*(j*inf.width+ii*npixperbyte+jj)+1] = cols[ncpal*colidx+1];
                        buffer[3*(j*inf.width+ii*npixperbyte+jj)+2] = cols[ncpal*colidx];
                        mask>>=inf.Colorbits;
                    }
                }
            }
            off += doff;
            if (ncomp>2)
            { // yes bill, colours are usually BGR aren't they
                int i;
                for(i=0; i<inf.width; i++)
                {
                    int ijw = i + j * inf.width;
                    unsigned char blu = buffer[3*ijw+0];
                    buffer[3*ijw+0] = buffer[3*ijw+2]; // swap order of colours
                    buffer[3*ijw+2] = blu;
                }
            }
        }
        free(imbuff); // free the on-disk storage

        fclose(fp);
    }
    else // else error in header
    {
        fprintf(stderr, "%s/%s: ERROR - (%s) issue with file header\n", __FILE__, __FUNCTION__, filename);
        fclose(fp);
        image->data = NULL;
        return (-1);
    }

    image->width = inf.width;
    image->height = inf.height;
    image->internalFormat = ncomp;
    image->data = buffer;

    setBMPImageData(image, (ncol)ncomp);

    return 0;
}
