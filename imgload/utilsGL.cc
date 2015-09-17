#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glu.h>
#include "imgload_internal.hh"

static void scaleImage(ImageStruct *, int, int, int);
static int computeNearestPowerOfTwo(int);
static void ensureValidSizeForTexturing(ImageStruct *, int);

/*********************************************************************************
 *
 *
 *********************************************************************************/
int createTextureFromImage(ImageStruct *image)
{
    int texture;
    GLint maxsize;

    glGenTextures(1, (GLuint *)&texture);
    if (texture < 0) return texture;

    // Scale the image to power of 2
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    ensureValidSizeForTexturing(image, maxsize);
   
    glBindTexture(GL_TEXTURE_2D, texture);

/* GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER let us change the way OpenGL magnifies and minifies a texture.
 * If we pass GL_LINEAR to the function, our texture would be interpolated using bilinear filtering (in other words, it'd appear smoothed).
 * If we pass GL_NEAREST then no smoothing would occur.
 * For the GL_TEXTURE_ENV (Texture Environment)If we pass GL_MODULATE we tell OpenGL to blend the texture with the base color of the object.
 * If we pass GL_DECAL or GL_REPLACE then the base color (and any lighting effects) would be replaced purely with the colors of the texture.
        - Courtesy http://paulyg.f2s.com/prog3.htm       
*/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
//    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexImage2D(image->target, image->level, image->internalFormat, image->width, image->height, image->border, image->format, image->type, image->data);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    return(texture);
}

/*********************************************************************************
 *
 * This function sets BMP image structure variables.
 *
 *********************************************************************************/
void setBMPImageData(ImageStruct *image, ncol ncomp)
{
    image->target = GL_TEXTURE_2D;
    image->level = 0;
    image->border = 0;
    image->type = GL_UNSIGNED_BYTE;
    switch (ncomp)
    {
        case NCOL_BW:
            image->format = GL_LUMINANCE;
            break;
        case NCOL_IA:
            image->format = GL_LUMINANCE_ALPHA;
            break;
        case NCOL_RGB:
            image->format = GL_RGB;
            break;
        case NCOL_RGBA:
            image->format = GL_RGBA;
            break;
        default:
            image->format = GL_RGB;
    }
}

/*********************************************************************************
 *
 * This function sets RGB image structure variables.
 *
 *********************************************************************************/
void setRGBImageData(ImageStruct *image, unsigned short x, unsigned short y, unsigned short z)
{
    image->target = GL_TEXTURE_2D;
    image->level = 0;
    image->border = 0;
    image->type = GL_UNSIGNED_BYTE;
    image->width = x;
    image->height = y;
    image->internalFormat = z;
    if (z == 3) 
        image->format = GL_RGB;
    else
        image->format = GL_RGBA;
}

/*********************************************************************************
 *
 * This function sets TGA image structure variables.
 *
 *********************************************************************************/
void setTGAImageData(ImageStruct *image, unsigned int bpp)
{
    image->target = GL_TEXTURE_2D;
    image->level = 0;
    image->border = 0;
    image->type = GL_UNSIGNED_BYTE;
    if (bpp == 24)
    {
        image->internalFormat = GL_RGB;
        image->format = GL_RGB;
    }
    else
    {
        image->internalFormat = GL_RGBA;
        image->format = GL_RGBA;
    }
}

/*********************************************************************************
 *
 *
 *********************************************************************************/
static unsigned int computeNumComponents(GLenum pixelFormat)
{
    switch(pixelFormat)
    {
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT): return 3;
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT): return 4;
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT): return 4;
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT): return 4;
        case(GL_COLOR_INDEX): return 1;
        case(GL_STENCIL_INDEX): return 1;
        case(GL_DEPTH_COMPONENT): return 1;
        case(GL_RED): return 1;
        case(GL_GREEN): return 1;
        case(GL_BLUE): return 1;
        case(GL_ALPHA): return 1;
        case(GL_RGB): return 3;
        case(GL_BGR): return 3;
        case(GL_RGBA): return 4;
        case(GL_BGRA): return 4;
        case(GL_LUMINANCE): return 1;
        case(GL_LUMINANCE_ALPHA): return 2;
        default:
            fprintf(stderr, "%s %d: Error pixelFormat = %x\n", __FILE__, __LINE__, (int)pixelFormat);
            return 0;
    }
}
            
/*********************************************************************************
 *
 *
 *********************************************************************************/
static unsigned int computePixelSizeInBits(GLenum format,GLenum type)
{
    switch(type)
    {
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT): 
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT): 
            return 4;
        
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT): 
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT):
            return 8;
    
        case(GL_BITMAP): 
            return computeNumComponents(format);
        
        case(GL_BYTE):
        case(GL_UNSIGNED_BYTE):  
            return 8*computeNumComponents(format);
        
        case(GL_SHORT):
        case(GL_UNSIGNED_SHORT):  
            return 16*computeNumComponents(format);
        
        case(GL_INT):
        case(GL_UNSIGNED_INT):
        case(GL_FLOAT):  
            return 32*computeNumComponents(format);
    
    
        case(GL_UNSIGNED_BYTE_3_3_2): 
        case(GL_UNSIGNED_BYTE_2_3_3_REV):  
            return 8;
        
        case(GL_UNSIGNED_SHORT_5_6_5):
        case(GL_UNSIGNED_SHORT_5_6_5_REV):
        case(GL_UNSIGNED_SHORT_4_4_4_4):
        case(GL_UNSIGNED_SHORT_4_4_4_4_REV):
        case(GL_UNSIGNED_SHORT_5_5_5_1):
        case(GL_UNSIGNED_SHORT_1_5_5_5_REV):
            return 16;
        
        case(GL_UNSIGNED_INT_8_8_8_8):
        case(GL_UNSIGNED_INT_8_8_8_8_REV):
        case(GL_UNSIGNED_INT_10_10_10_2):
        case(GL_UNSIGNED_INT_2_10_10_10_REV): 
            return 32;

        default: 
        {
            fprintf(stderr, "%s %d:error type = %d\n", __FILE__, __LINE__, (int)type);
            return 0;
        }
    }
}

/*********************************************************************************
 *
 *
 *********************************************************************************/
static unsigned int computeRowWidthInBytes(int width, GLenum pixelFormat, GLenum type, int packing)
{
    unsigned int pixelSize = computePixelSizeInBits(pixelFormat, type);
    int widthInBits = width * pixelSize;
    int packingInBits = packing * 8;
    unsigned int widthInBytes = (widthInBits/packingInBits + ((widthInBits%packingInBits)?1:0)) * packing;

    return widthInBytes;
}

/*********************************************************************************
 *
 *
 *********************************************************************************/
static void scaleImage(ImageStruct *image, int s,int t,int r)
{
    int newDataType = image->type;
    
    if (image->width==s && image->height==t && image->level==r) return; // no need to scale image

    if (!(image->data))
    {
        fprintf(stderr, "%s %d: Cannot scale NULL image.\n", __FILE__, __LINE__);
        return;
    }

    if (r!=1)
    {
        fprintf(stderr, "%s %d: Scaling of volumes not implemented. r=%d.\n",__FILE__, __LINE__, r);
        return;
    }

    image->packing = 1;
    unsigned int newTotalSize = computeRowWidthInBytes(s,image->format,newDataType,image->packing)*t;
//    fprintf(stderr, "%s/%s compute row width in bytes %d\n",__FILE__,__LINE__,newTotalSize);

//    fprintf(stderr, "%s/%s allocate data for new image size... ",__FILE__,__LINE__);
    // need to sort out what size to really use...
//    unsigned char* newData = new unsigned char [newTotalSize];
    unsigned char* newData = (unsigned char*)malloc(newTotalSize);
//    fprintf(stderr, "done %d\n",newTotalSize);
//    unsigned char* newData = (unsigned char*)malloc(image->width * image->height * 3);
//    fprintf(stderr, "done %d\n",image->width * image->height * 3);

    if (!newData)
    {
        fprintf(stderr, "%s %d: %s did not succeed : out of memory. %d\n", __FILE__,__LINE__, __FUNCTION__,  newTotalSize);
        return;
    }

    glPixelStorei(GL_PACK_ALIGNMENT,image->packing);
    glPixelStorei(GL_UNPACK_ALIGNMENT,image->packing);

    GLint status = gluScaleImage(image->format,
        image->width,
        image->height,
        image->type,
        image->data,
        s,
        t,
        newDataType,
        newData);

    if (status == 0)
    {
        image->width = s;
        image->height = t;
        image->type = newDataType;
        image->data = (unsigned char*)realloc(image->data, newTotalSize);
        memcpy(image->data, newData, newTotalSize);
    }
    else fprintf(stderr, "%s %d: %s did not succeed : %s\n", __FILE__,__LINE__, __FUNCTION__,  (char *)gluErrorString((GLenum)status));
}

/*********************************************************************************
 *
 *
 *********************************************************************************/
static int computeNearestPowerOfTwo(int s)
{
    float bias = 0.5;
    if ((s & (s-1))!=0)
    {
        // it isn't so lets find the closest power of two.
        // yes, logf and powf are slow, but this code should
        // only be called during scene graph initilization,
        // if at all, so not critical in the greater scheme.
        float p2 = logf((float)s)/logf(2.0f);
        float rounded_p2 = floorf(p2+bias);
        s = (int)(powf(2.0f,rounded_p2));
    }
    return s;
}

/*********************************************************************************
 *
 *
 *********************************************************************************/
static void ensureValidSizeForTexturing(ImageStruct *image, int maxTextureSize)
{
    int new_s = computeNearestPowerOfTwo(image->width);
    int new_t = computeNearestPowerOfTwo(image->height);
    
    if (new_s>maxTextureSize) new_s = maxTextureSize;
    if (new_t>maxTextureSize) new_t = maxTextureSize;
    
    if (new_s!=image->width || new_t!=image->height) scaleImage(image, new_s, new_t, 1);
}
