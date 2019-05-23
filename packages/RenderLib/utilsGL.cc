#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#ifndef IOS_BUILD
#include <GL/glu.h>
#else
#include <OpenGLES/ES1/glext.h>
#endif
#include "basicutils/msg.hh"
#include "imgload.hh"

#ifndef IOS_BUILD
static void ensureValidSizeForTexturing(ImageStruct *, GLint, GLenum);
#endif


int createTextureFromImage(ImageStruct *image)
{
    int texture;
    GLenum format;
    GLint bytesPerPixel;

    glGenTextures(1, (GLuint *)&texture);
    if (texture < 0) return texture;

    switch (image->pixelspec)
    {
        case PixelLuminance:
            format = GL_LUMINANCE;
            bytesPerPixel = 1;
            break;
        case PixelLuminanceAlpha:
            format = GL_LUMINANCE_ALPHA;
            bytesPerPixel = 2;
            break;
        case PixelRGB:
            format = GL_RGB;
            bytesPerPixel = 3;
            break;
        case PixelRGBA:
            format = GL_RGBA;
            bytesPerPixel = 4;
            break;
        default:
            warning_msg("Invalid pixel specification: " << image->pixelspec);
            format = GL_RGB;
            bytesPerPixel = 3;
    }

#ifndef IOS_BUILD // iOS 2+ provides NPOT
    // Scale the image to power of 2 (may not be necessary)
    ensureValidSizeForTexturing(image, bytesPerPixel, format);
#endif

    glBindTexture(GL_TEXTURE_2D, texture);

// GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER define how OpenGL magnifies and minifies a texture:
//   GL_LINEAR tells OpenGL to interpolate the texture using bilinear filtering (in other words, to smooth it).
//   GL_NEAREST tells OpenGL to not do any smoothing.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
// For GL_TEXTURE_ENV:
//   GL_MODULATE tells OpenGL to blend the texture with the base color of the object.
//   GL_DECAL or GL_REPLACE tells OpenGL to replace the base color (and any lighting effects) purely with the colors of the texture.
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE, image->data);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    return texture;
}


#ifndef IOS_BUILD
static int computeNearestPowerOfTwo(int val)
{
    if (val & (val - 1))
    {
        float p2 = logf((float)val) / 0.69314718; /* logf(2.0f) = 0.69314718 */
        float rounded_p2 = floorf(p2 + 0.5);
        val = (int)(powf(2.0f, rounded_p2));
    }
    return val;
}


static void ensureValidSizeForTexturing(ImageStruct *image, GLint bytesPerPixel, GLenum format)
{
    if (!(image->data))
    {
        warning_msg("Cannot scale NULL image");
        return;
    }

    GLint maxTextureSize;
    int newW = computeNearestPowerOfTwo(image->width);
    int newH = computeNearestPowerOfTwo(image->height);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    if (newW > maxTextureSize) newW = maxTextureSize;
    if (newH > maxTextureSize) newH = maxTextureSize;

    if (newW != image->width || newH != image->height)
    {
        unsigned int newTotalSize = newW * newH * bytesPerPixel;
        unsigned char *newData = (unsigned char *)malloc(newTotalSize);

        if (!newData)
        {
            warning_msg("Unable to malloc " << newTotalSize << " bytes");
            return;
        }

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        GLint status = gluScaleImage(format, image->width, image->height, GL_UNSIGNED_BYTE, image->data, newW, newH, GL_UNSIGNED_BYTE, newData);

        if (!status)
        {
            image->width = newW;
            image->height = newH;
            image->data = (unsigned char *)realloc(image->data, newTotalSize);
            memcpy(image->data, newData, newTotalSize);
        }
        else warning_msg("gluScaleImage failed: " << (char *)gluErrorString((GLenum)status));
    }
}
#endif
