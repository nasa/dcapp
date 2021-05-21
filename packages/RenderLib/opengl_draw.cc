#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <assert.h>
#include <GL/gl.h>
#ifdef NPOT_NEEDED
#include <GL/glu.h>
#endif
#ifdef IOS_BUILD
#include <OpenGLES/ES1/glext.h>
#endif
#include "basicutils/msg.hh"
#include "fontlib.hh"
#include "texturelib.hh"

#ifdef NPOT_NEEDED
static GLint maxTextureSize;

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

static void convertToNearestPowerOfTwo(tdTexture *textureID)
{
    if (!(textureID->data))
    {
        warning_msg("Cannot scale NULL image");
        return;
    }

    int newW = computeNearestPowerOfTwo(textureID->width);
    int newH = computeNearestPowerOfTwo(textureID->height);

    if (newW > maxTextureSize) newW = maxTextureSize;
    if (newH > maxTextureSize) newH = maxTextureSize;

    if (newW != textureID->width || newH != textureID->height)
    {
        unsigned int newTotalSize = newW * newH * textureID->bytesPerPixel;
        unsigned char *newData = (unsigned char *)malloc(newTotalSize);

        if (!newData)
        {
            warning_msg("Unable to malloc " << newTotalSize << " bytes");
            return;
        }

        GLint status = gluScaleImage((GLenum)(textureID->pixelspec), textureID->width, textureID->height, GL_UNSIGNED_BYTE, textureID->data, newW, newH, GL_UNSIGNED_BYTE, newData);

        if (!status)
        {
            textureID->width = newW;
            textureID->height = newH;
            textureID->data = (unsigned char *)realloc(textureID->data, newTotalSize);
            memcpy(textureID->data, newData, newTotalSize);
        }
        else warning_msg("gluScaleImage failed: " << (char *)gluErrorString((GLenum)status));

        free(newData);
    }
}
#endif

void init_window(void)
{
    glClearColor(0, 0, 0, 0);          // Clear the window to black
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);                // Set the clear value for the depth buffer
    glDepthFunc(GL_LEQUAL);            // Set the depth buffer comparison method
#ifdef NPOT_NEEDED
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
#endif

    // Establish graphical settings that are universal among the dcapp visual primitives...

    // Make sure that all byte packing and unpacking byte aligned
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Enable blending for elements with alpha values
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

// This glViewport call is problematic on MacOS.  It apparently requires width and height in the backing bounds frame,
// while mouse location is calculated in pixels.  We can get the data in the backing bounds frame if needed (see comment
// in the windowReconfigured routine in tdAdapter.mm), but it would need to be sent separately from the new window size.
// However, MacOS (starting with Catalina) seems to update glViewport automatically, so the simple solution is to not
// call it here for modern MacOS systems.
#ifdef CATALINA
void reshape_window(int, int) { }
#else
void reshape_window(int w, int h)
{
    glViewport(0, 0, w, h);
}
#endif

void setup_panel(float x, float y, float red, float green, float blue, float alpha)
{
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, x, 0, y, -1, 1);
//    glMatrixMode(GL_MODELVIEW);
//    glColor4f(1, 1, 1, 1);
}

// allocate textures in blocks, which is much faster on Linux that allocating them one at a time
#define TEXTURE_BLOCKS 100
unsigned getTexture(void)
{
    static unsigned gltextures[TEXTURE_BLOCKS], mycount = 0;
    unsigned retval;

    if (!mycount) glGenTextures(TEXTURE_BLOCKS, gltextures);
    retval = gltextures[mycount];
    mycount++;
    if (mycount == TEXTURE_BLOCKS) mycount = 0;

    return retval;
}

// TODO: create_and_load_glyph is very similar to create_texture and load_texture - consider combining
void create_and_load_glyph(unsigned int *mytexture, void *pixels)
{
    *mytexture = getTexture();
    glBindTexture(GL_TEXTURE_2D, *mytexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// GL_ALPHA could be replaced by a pixelspec value
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 64, 64, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
}

void create_texture(tdTexture *textureID)
{
    textureID->setID(getTexture());
}

void load_texture(tdTexture *textureID)
{
    if (textureID->isValid())
    {
        glBindTexture(GL_TEXTURE_2D, textureID->getID());

#ifdef NPOT_NEEDED
        // Scale the image to power of 2 (OpenGL programming encourages, and used to require, this)
        if (textureID->convertNPOT) convertToNearestPowerOfTwo(textureID);
#endif

        if (textureID->smooth)
        {
            // GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER define how a texture is magnified and minified:
            //   GL_LINEAR interpolates the texture using bilinear filtering to smooth it
            //   GL_NEAREST doesn't do any smoothing
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            // For GL_TEXTURE_ENV:
            //   GL_MODULATE blends the texture with the base color of the object
            //   GL_DECAL or GL_REPLACE replaces the base color purely with the colors of the texture
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)(textureID->pixelspec), textureID->width, textureID->height, 0, (GLenum)(textureID->pixelspec), GL_UNSIGNED_BYTE, textureID->data);
    }
}

void draw_string(float xpos, float ypos, float size, float red, float green, float blue, float alpha, tdFont *fontID, flMonoOption mono, const std::string &instring)
{
    float scale = size / fontID->getBaseSize();
    glColor4f(red, green, blue, alpha);
    glPushMatrix();
        glEnable(GL_TEXTURE_2D);                                       // Enable Texture Mapping
        glTranslatef(xpos, ypos - (fontID->getDescender() * scale), 0);
        glScalef(scale, scale, 0);
        fontID->render(instring, mono);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// TODO: draw_glyph and draw_image are very similar in function - consider combining if it doesn't impact efficiency
void draw_glyph(unsigned int mytexture, float startx, float starty, float kern, float advance)
{
    static GLfloat verts[12] = { 0,64,0, 0,0,0, 64,64,0, 64,0,0 };
    static GLfloat uvs[8] = { 0,0, 0,1, 1,0, 1,1 };

    glTranslatef(kern + startx, starty, 0);

    glBindTexture(GL_TEXTURE_2D, mytexture);
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ginfo->width, ginfo->height, GL_ALPHA, GL_UNSIGNED_BYTE, ginfo->bitmap);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glTranslatef(advance - startx, -starty, 0);
}

void draw_image(tdTexture *textureID, float w, float h)
{
    if (textureID)
    {
        if (textureID->isValid())
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID->getID());
            glColor4f(1, 1, 1, 1);
            glBegin(GL_QUADS);
                glTexCoord2f(0, 0);
                glVertex3f(0, 0, 0);
                glTexCoord2f(1, 0);
                glVertex3f(w, 0, 0);
                glTexCoord2f(1, 1);
                glVertex3f(w, h, 0);
                glTexCoord2f(0, 1);
                glVertex3f(0, h, 0);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }
    }
}

void get_image_pixel(unsigned char rgba[], tdTexture *textureID, float xpct, float ypct)
{
    GLint textureWidth, textureHeight;
    unsigned int pixx, pixy, i;

    glBindTexture(GL_TEXTURE_2D, textureID->getID());
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);

    unsigned char mypixels[textureHeight][textureWidth][textureID->bytesPerPixel];
    glGetTexImage(GL_TEXTURE_2D, 0, (GLenum)(textureID->pixelspec), GL_UNSIGNED_BYTE, (GLvoid *)mypixels);

    pixx = (unsigned int)(xpct * textureWidth);
    pixy = (unsigned int)(ypct * textureHeight);

    for (i=0; i<(textureID->bytesPerPixel); i++) rgba[i] = mypixels[pixy][pixx][i];
}

void container_start(float refx, float refy, float delx, float dely, float scalex, float scaley, float rotate)
{
    glPushMatrix();
        glTranslatef(refx, refy, 0);
        glRotatef(rotate, 0, 0, 1);
        glScalef(scalex, scaley, 1);
        glTranslatef(-delx/scalex, -dely/scaley, 0);
}

void container_end(void)
{
    glPopMatrix();
}

void rotate_start(float rot)
{
    glPushMatrix();
    glRotatef(rot, 0, 0, 1);
}

void rotate_end(void)
{
    glPopMatrix();
}

#if 0
void translate_start(float x, float y)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
}

void translate_end(void)
{
    glPopMatrix();
}
#endif

void draw_line(const std::vector<float> &pntsA, float linewidth, float red, float green, float blue, float alpha, uint16_t pattern, int factor)
{
    glPushMatrix();

    glLineWidth(linewidth);
    glColor4f(red, green, blue, alpha);
    glLineStipple(factor, pattern);
    glEnable(GL_LINE_STIPPLE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pntsA.data());
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<int>(pntsA.size()/2));
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
    glDisable(GL_LINE_STIPPLE);
}

void draw_polygon(const std::vector<float> &pntsA, float red, float green, float blue, float alpha)
{
    std::vector<float> localPointsL;

    for (size_t iL=0; iL< pntsA.size()-1; iL+=2)
    {
        localPointsL.push_back(pntsA[iL]);
        localPointsL.push_back(pntsA[iL+1]);
    }

    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, localPointsL.data());
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<int>(localPointsL.size()/2));
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_filled_triangles(const std::vector< float >&pntsA, float red, float green, float blue, float alpha)
{
    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pntsA.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(pntsA.size() / 2));
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_quad(const std::vector<float> &pntsA, float red, float green, float blue, float alpha)
{
    assert(pntsA.size() == 8);
    std::vector< float > localPointsL;

    // tri1
    localPointsL.push_back( pntsA[0] );
    localPointsL.push_back( pntsA[1] );

    localPointsL.push_back( pntsA[2] );
    localPointsL.push_back( pntsA[3] );

    localPointsL.push_back( pntsA[4] );
    localPointsL.push_back( pntsA[5] );

    // tri1
    localPointsL.push_back( pntsA[0] );
    localPointsL.push_back( pntsA[1] );

    localPointsL.push_back( pntsA[4] );
    localPointsL.push_back( pntsA[5] );

    localPointsL.push_back( pntsA[6] );
    localPointsL.push_back( pntsA[7] );

    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, localPointsL.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(localPointsL.size()/2));
    glDisableClientState(GL_VERTEX_ARRAY);
}

void circle_outline(float cx, float cy, float r, int num_segments, float red, float green, float blue, float alpha, float linewidth, uint16_t linepattern, int linefactor)
{
    float theta = 2 * 3.1415926 / (float)num_segments;
    float c = cosf(theta); // precalculate the sine and cosine
    float s = sinf(theta);
    float t;
    float x = r; // we start at angle = 0
    float y = 0;
    int i;

    glColor4f(red, green, blue, alpha);
    glLineWidth(linewidth);
    glLineStipple(linefactor, linepattern);
    glEnable(GL_LINE_STIPPLE);
    glEnable(GL_LINE_SMOOTH);
    glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

    glBegin(GL_LINE_LOOP);
    for (i = 0; i < num_segments; i++)
    {
        glVertex2f(x + cx, y + cy); // output vertex

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_LINE_STIPPLE);
}

void circle_fill(float cx, float cy, float r, int num_segments, float red, float green, float blue, float alpha)
{
    float theta = 2 * 3.1415926 / (float)num_segments;
    float c = cosf(theta); // precalculate the sine and cosine
    float s = sinf(theta);
    float t;
    float x = r; // we start at angle = 0
    float y = 0;
    int i;

    glColor4f(red, green, blue, alpha);
    glBegin(GL_POLYGON);
    for (i = 0; i < num_segments; i++)
    {
        glVertex2f(x + cx, y + cy); // output vertex

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    glEnd();
}

void draw_textured_sphere(float x, float y, const std::vector<float> &pointsA, float radiusA, tdTexture *textureID, float roll, float pitch, float yaw)
{
    if (textureID->isValid())
    {
    //  glEnable(GL_DEPTH_TEST);                    // Enables Depth Testing
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
        glBindTexture(GL_TEXTURE_2D, textureID->getID());
        glColor4f(1, 1, 1, 1);

        glPushMatrix();
            glTranslatef(x, y, 0);
            glScalef(radiusA, radiusA, 1.0);

            /* First, orient the sphere for correct rendering at roll=0, pitch=0, and yaw=0 */
            glRotatef(180, 0, 0, 1);
            glRotatef(-90, 1, 0, 0);

            /* Then, orient the sphere according to the roll, pitch, and yaw values */
            glRotatef(-roll,  0, 1, 0);
            glRotatef( yaw,   0, 0, 1);
            glRotatef(-pitch, 1, 0, 0);

            /* Draw the sphere */
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 5, pointsA.data() + 3);
            glVertexPointer(3, GL_FLOAT, sizeof(float) * 5, pointsA.data());
            glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(pointsA.size() / 5));
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glPopMatrix();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
    //  glDisable(GL_DEPTH_TEST);                    // disables Depth Testing
    }
}
