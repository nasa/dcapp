#ifndef _OPENGL_DRAW_HH_
#define _OPENGL_DRAW_HH_

#include <GL/gl.h>

#define PixelUnknown        (-1)
#define PixelAlpha          GL_ALPHA
#define PixelLuminance      GL_LUMINANCE
#define PixelLuminanceAlpha GL_LUMINANCE_ALPHA
#define PixelRGB            GL_RGB
#define PixelRGBA           GL_RGBA

#define translate_start(a, b) glPushMatrix(); glTranslatef(a, b, 0);
#define translate_end() glPopMatrix();

#endif
