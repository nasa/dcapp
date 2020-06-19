/*********************************************************************************
 * Create ImageStruct data from the contents of a bitmap file
 *********************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "basicutils/msg.hh"
#include "texturelib.hh"

// BMP format bits - at start of file is 512 bytes of pure garbage
#define BMPfiletype 0x4d42 // file type identifier -- literally 'B''M'

typedef struct
{
    short FileType;             // must be BMPfiletype
    unsigned short filesize[2]; // size of the whole file
    short reserved[2];          // reserved for future purposes
    unsigned short offset[2];   // offset of the image (in bytes)
} BMPheader;

typedef struct
{
    int width;        // width of the image in pixels
    int height;       // height of the image in pixels
    short planes;     // number of planes (always 1)
    short ColorBits;  // number of bits used to describe color in each pixel
    int compression;  // compression used
    int ImageSize;    // image size in bytes
    int XpixPerMeter; // pixels per meter in X
    int YpixPerMeter; // pixels per meter in Y
    int ColorsUsed;   // number of colors used
    int Important;    // number of "important" colors
} BMPinfo;

/* byte order workarounds */
static void swapbyte_L(int *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[3];
    vv[3]=tmp;
    tmp=vv[1];
    vv[1]=vv[2];
    vv[2]=tmp;
}

static void swapbyte_US(unsigned short *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[1];
    vv[1]=tmp;
}

static void swapbyte_S(short *i)
{
    char *vv=(char *)i;
    char tmp=vv[0];
    vv[0]=vv[1];
    vv[1]=tmp;
}


int tdTexture::loadBMP(void)
{
    unsigned char *buffer = 0x0;
    int filelen;
    int ncomp = 0;
    bool swap = false;
    BMPheader hd;
    BMPinfo inf;

    FILE *fp = fopen(this->filename.c_str(), "rb");
    if (!fp)
    {
        warning_msg("The file " << this->filename << " does not exist");
        this->data = 0x0;
        return (-1);
    }

    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp); // actual size of the file
    fseek(fp, 0, SEEK_SET);

    fread((char *)&hd, sizeof(BMPheader), 1, fp);

    if (hd.FileType != BMPfiletype)
    {
        swapbyte_S(&(hd.FileType));
        swap = true;
    }

    if (hd.FileType == BMPfiletype)
    {
        int infsize; // size of BMPinfo in bytes
        unsigned char *cols = 0x0; // dynamic color palette
        unsigned char *imbuff = 0x0;

        fread((char *)&infsize, sizeof(int), 1, fp);
        if (swap) swapbyte_L(&infsize);

        unsigned char *hdr = (unsigned char *)malloc(infsize); // memory for the new header

        fread((char *)hdr, 1,infsize-sizeof(int), fp);

        int hsiz = sizeof(inf);

        if (infsize <= hsiz) hsiz = infsize;
        memcpy(&inf, hdr, hsiz);
        free(hdr);

        if (swap)
        {
            swapbyte_US(&hd.filesize[0]);
            swapbyte_US(&hd.filesize[1]);
            swapbyte_S(&inf.ColorBits);
            swapbyte_L(&inf.width);
            swapbyte_L(&inf.height);
            swapbyte_L(&inf.ImageSize);
        }
        if (infsize == 12) // os2
        {
            int wd = inf.width & 0xffff; // shorts replace ints
            int ht = inf.width >> 16;
            int npln = inf.height & 0xffff; // number of planes
            int cbits = inf.height >> 16;
            inf.width = wd;
            inf.height = ht;
            inf.planes = npln;
            inf.ColorBits = cbits;
            inf.ColorsUsed = (int)pow(2.0, (double)inf.ColorBits); // infer the colors
        }

        // previous size calculation, see new calcs below.

        int size = (hd.filesize[1] * 65536) + hd.filesize[0];
        if (size == 0) size = filelen;

        int ncpal = 4; // default number of colors per palette entry
        size -= sizeof(BMPheader) + infsize;
        if (inf.ImageSize < size) inf.ImageSize = size;
        imbuff = (unsigned char *)malloc(inf.ImageSize);

        if (!imbuff)
        {
            warning_msg("Memory allocation error");
            this->data = 0x0;
            return (-1);
        }

        if (fread((char *)imbuff, sizeof(unsigned char), inf.ImageSize, fp) != (size_t)inf.ImageSize)
        {
            warning_msg("Problem retrieving image buffer from " << this->filename);
            this->data = 0x0;
            free(imbuff);
            return (-1);
        }

        int ncolors = inf.ColorBits/8;
        switch (ncolors)
        {
        case 1:
            this->pixelspec = PixelLuminance;
            ncomp = 1;            // this is a 256 color, paletted image
            inf.ColorBits = 8;    // number of bits per index
            inf.ColorsUsed = 256; // number of colors used
            cols=imbuff;          // color palette address - uses 4 bytes/color
            break;
        case 2:
            this->pixelspec = PixelLuminanceAlpha;
            ncomp = 2;
            break;
        case 3:
            this->pixelspec = PixelRGB;
            ncomp = 3;
            break;
        case 4:
            this->pixelspec = PixelRGBA;
            ncomp = 4;
            break;
        default:
            this->pixelspec = PixelUnknown;
            cols = imbuff;                                 // color palette address - uses 4 bytes/colour
            if (infsize == 12 || infsize == 64) ncpal = 3; // OS2 uses 3 colors per palette entry
            else ncpal = 4;                                // Windows uses 4
        }

        if (ncomp > 1)
            buffer = (unsigned char *)malloc(inf.width * inf.height * ncomp);
        else
            buffer = (unsigned char *)malloc(inf.width * inf.height * 3); // default to full colour

        unsigned int off = 0;
        unsigned int rowbytes = ncomp * sizeof(unsigned char) * inf.width;
        unsigned int doff = ((rowbytes + 3)/4)*4; // doff is rowbytes rounded up to the next multiple of 4
        int j;

        for (j = 0; j < inf.height; j++)
        {
            if (ncomp > 1) memcpy(buffer + (j * rowbytes), imbuff + off, rowbytes); // pack bytes closely
            else
            { // find from the palette...
                unsigned char *imptr = imbuff + (inf.ColorsUsed * ncpal); // add size of the palette
                int npixperbyte = 8/inf.ColorBits;
                int ii;
                for (ii = 0; ii < inf.width/npixperbyte; ii++)
                {
                    unsigned char mask = 0x00; // masked with index to extract color bits
                    unsigned char byte = imptr[(j * inf.width / npixperbyte) + ii];
                    int jj;
                    for (jj = 0; jj < inf.ColorBits; jj++) mask |= (0x80 >> jj); // fill high-end bits
                    for (jj = 0; jj < npixperbyte; jj++)
                    {
                        int colidx = (byte & mask) >> ((npixperbyte - 1 - jj) * inf.ColorBits);
                        buffer[3 * (j * inf.width + ii * npixperbyte + jj) + 0] = cols[ncpal * colidx + 2];
                        buffer[3 * (j * inf.width + ii * npixperbyte + jj) + 1] = cols[ncpal * colidx + 1];
                        buffer[3 * (j * inf.width + ii * npixperbyte + jj) + 2] = cols[ncpal * colidx];
                        mask >>= inf.ColorBits;
                    }
                }
            }
            off += doff;
            if (ncomp > 2)
            {
                int i;
                // color order in a bitmap file is BGR, so swap red and blue values
                for (i=0; i<inf.width; i++)
                {
                    int ijw = 3 * (i + (j * inf.width));
                    unsigned char blu = buffer[ijw];
                    buffer[ijw] = buffer[ijw + 2];
                    buffer[ijw + 2] = blu;
                }
            }
        }
        
        free(imbuff);
        fclose(fp);
    }
    else
    {
        warning_msg("Issue with file header in " << this->filename);
        fclose(fp);
        this->data = 0x0;
        return (-1);
    }

    this->width = inf.width;
    this->height = inf.height;
    this->data = buffer;

    return 0;
}
