#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <assert.h>
#include <GL/gl.h>
#include "fontlib/fontlib.hh"


/*********************************************************************************
 *
 * This function is a general OpenGL initialization function. It sets all of the
 * initial parameters.
 *
 *********************************************************************************/
void graphics_init(void)
{
    glClearColor(0, 0, 0, 0);          // Clear the window to black.
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);                // Depth Buffer Setup
    glDepthFunc(GL_LEQUAL);            // The Type Of Depth Testing To Do
}

void setup_panel(float x, float y, int near, int far, float red, float green, float blue, float alpha)
{
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, x, 0, y, near, far);
    glMatrixMode(GL_MODELVIEW);
    glColor4f(1, 1, 1, 1);
}

void reshape_viewport(int w, int h)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, w, h);            // Set the viewport to the whole window.
}

void init_texture(unsigned int *textureID)
{
    glGenTextures(1, (GLuint *)textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Make sure that byte unpacking (for PixelStream, etc.) is properyly byte aligned
}

void set_texture(unsigned int textureID, int width, int height, void *pixels)
{
    glBindTexture(GL_TEXTURE_2D, textureID);
// GL_LINEAR interpolates using bilinear filtering (smoothing). GL_NEAREST specifies no smoothing.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

void draw_image(unsigned int textureID, float w, float h)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, textureID);
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
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void get_image_pixel_RGBA(unsigned char rgba[], unsigned int textureID, float xpct, float ypct)
{
    GLint textureWidth, textureHeight;
    int pixx, pixy, i;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);
    unsigned char mypixels[textureHeight][textureWidth][4];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)mypixels);

    pixx = (int)(xpct * textureWidth);
    pixy = (int)(ypct * textureHeight);

    for (i=0; i<4; i++) rgba[i] = mypixels[pixy][pixx][i];
}

float get_string_width(flFont *fontID, float size, flMonoOption mono, const char *string)
{
    if (!fontID || !string) return 0;
    return fontID->getAdvance(string, mono) * size / fontID->getBaseSize();
}

void draw_string(float xpos, float ypos, float size, float red, float green, float blue, float alpha, flFont *fontID, flMonoOption mono, const char *string)
{
    if (!fontID || !string) return;
    float scale = size / fontID->getBaseSize();
    glColor4f(red, green, blue, alpha);
    glPushMatrix();
        glEnable(GL_TEXTURE_2D);                                       // Enable Texture Mapping
        glEnable(GL_BLEND);                                            // Blend the transparent part with the background
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glTranslatef(xpos, ypos - (fontID->getDescender() * scale), 0);
        glScalef(scale, scale, 0);
        fontID->render(string, mono);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void container_start(float refx, float refy, float delx, float dely, float scalex, float scaley, float rotate)
{
    glPushMatrix();
        glTranslatef(refx, refy, 0);
        glScalef(scalex, scaley, 1);
        glRotatef(rotate, 0, 0, 1);
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

void translate_start(float x, float y)
{
    glPushMatrix();
        glTranslatef(x, y, 0);
}

void translate_end(void)
{
    glPopMatrix();
}

void draw_line(const std::vector<float> &pntsA, float linewidth, float red, float green, float blue, float alpha)
{
    glPushMatrix();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(linewidth);
    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pntsA.data());
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<int>(pntsA.size()/2));
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);

    glPopMatrix();
}

void draw_polygon( const std::vector<float> &pntsA, float red, float green, float blue, float alpha)
{
    std::vector<float> localPointsL;

    for (size_t iL=0; iL< pntsA.size()-1; iL+=2)
    {
        localPointsL.push_back(pntsA[iL]);
        localPointsL.push_back(pntsA[iL+1]);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, localPointsL.data());
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<int>(localPointsL.size()/2));
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

void draw_filled_triangles( const std::vector< float >&pntsA, float red, float green, float blue, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pntsA.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(pntsA.size() / 2));
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

void draw_quad( const std::vector<float> &pntsA, float red, float green, float blue, float alpha)
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(red, green, blue, alpha);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, localPointsL.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(localPointsL.size()/2));
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

void circle_outline(float cx, float cy, float r, int num_segments, float red, float green, float blue, float alpha, float linewidth)
{
    float theta = 2 * 3.1415926 / (float)num_segments;
    float c = cosf(theta); // precalculate the sine and cosine
    float s = sinf(theta);
    float t;
    float x = r; // we start at angle = 0
    float y = 0;
    int i;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glLineWidth(linewidth);
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
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    glDisable(GL_BLEND);
}

void draw_textured_sphere(float x, float y, const std::vector<float> &pointsA, float radiusA, int textureID, float roll, float pitch, float yaw)
{
//  glEnable(GL_DEPTH_TEST);                    // Enables Depth Testing
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glColor4f(1.0, 1.0, 1.0, 1.0);

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

void addPoint(std::vector<float> &listA, float xA, float yA)
{
    listA.push_back(xA);
    listA.push_back(yA);
}

void addPoint(std::vector<float> &listA, float xA, float yA, float zA)
{
    listA.push_back(xA);
    listA.push_back(yA);
    listA.push_back(zA);
}

void addPoint(std::vector<float> &listA, float xA, float yA, float zA, float uA, float vA)
{
    listA.push_back(xA);
    listA.push_back(yA);
    listA.push_back(zA);
    listA.push_back(uA);
    listA.push_back(vA);
}
