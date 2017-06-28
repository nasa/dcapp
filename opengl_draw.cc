#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#ifndef IOS_BUILD
#include <GL/glu.h>
#else
#include <OpenGLES/ES1/glext.h>
#include <vector>
#endif
#include "fontlib/fontlib.hh"

#include <iostream>
void CheckError( const char *idA )
{
    auto errorL = glGetError();
    if( errorL )
        std::cout << "ErrorL(" << idA << ") = " << errorL << std::endl;
}

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
#ifndef IOS_BUILD
    glClearDepth(1.0f);                // Depth Buffer Setup
#else
    glClearDepthf( 1.0f );
#endif
    glDepthFunc(GL_LEQUAL);            // The Type Of Depth Testing To Do
}

/*********************************************************************************
 *
 * This function handles reshaping a window.
 *
 *********************************************************************************/
void reshape(int w, int h)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, w, h);            // Set the viewport to the whole window.
}

void setup_panel(float x, float y, int near, int far, float red, float green, float blue, float alpha)
{
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifndef IOS_BUILD
    glOrtho(0, x, 0, y, near, far);
#else
    glOrthof(0, x, 0, y, near, far );
#endif

    glMatrixMode(GL_MODELVIEW);
    glColor4f(1, 1, 1, 1);
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
    
#ifndef IOS_BUILD
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
#else
    const GLfloat vertices[8]   = { 0,h, 0,0, w,h, w,0 };
    const GLfloat uvs[8]        = { 0,1, 0,0, 1,1, 1,0 };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer( 2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void get_image_pixel_RGBA(unsigned char rgba[], unsigned int textureID, float xpct, float ypct)
{
#ifndef IOS_BUILD
    GLint textureWidth, textureHeight;
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);
    int pixx, pixy, i;
    
    unsigned char mypixels[textureHeight][textureWidth][4];
    pixx = (int)(xpct * textureWidth);
    pixy = (int)(ypct * textureHeight);
    
    
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)mypixels);
    
    for (i=0; i<4; i++) rgba[i] = mypixels[pixy][pixx][i];
#endif
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

void line_start(float linewidth, float red, float green, float blue, float alpha)
{
#ifndef IOS_BUILD
    glPushMatrix();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(linewidth);
        glColor4f(red, green, blue, alpha);

        glBegin(GL_LINE_STRIP);
#endif
}

void line_end(void)
{
#ifndef IOS_BUILD
        glEnd();
    glPopMatrix();
#endif
}

void polygon_outline_start(float linewidth, float red, float green, float blue, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glLineWidth(linewidth);
#ifndef IOS_BUILD
    glBegin(GL_LINE_LOOP);
#endif
}

void polygon_outline_end(void)
{
#ifndef IOS_BUILD
    glEnd();
#endif
}

void polygon_fill_start(float red, float green, float blue, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef IOS_BUILD
    glColor4f(red, green, blue, alpha);
    glBegin(GL_POLYGON);
#endif
}

void polygon_fill_end(void)
{
#ifndef IOS_BUILD
    glEnd();
#endif
}

void gfx_vertex(float x, float y)
{
#ifndef IOS_BUILD
    glVertex2f(x, y);
#endif
}

void rectangle_outline(float linewidth, float red, float green, float blue, float alpha, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(red, green, blue, alpha);

    glLineWidth(linewidth);
#ifndef IOS_BUILD
    glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
    glEnd();
#else
    GLfloat vertices[4][2];
    
    vertices[0][0]  = x1;
    vertices[0][1]  = y1;
    vertices[1][0]  = x2;
    vertices[1][1]  = y2;
    vertices[2][0]  = x3;
    vertices[2][1]  = y3;
    vertices[3][0]  = x4;
    vertices[3][1]  = y4;
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer( 2, GL_FLOAT, 0, vertices);
    glDrawArrays( GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
#endif
}

void rectangle_fill(float red, float green, float blue, float alpha, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    
#ifndef IOS_BUILD
    glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
    glEnd();
#else
    GLfloat vertices[4][2];
    
    vertices[0][0]  = x1;
    vertices[0][1]  = y1;
    vertices[1][0]  = x2;
    vertices[1][1]  = y2;
    vertices[2][0]  = x4;
    vertices[2][1]  = y4;
    vertices[3][0]  = x3;
    vertices[3][1]  = y3;
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer( 2, GL_FLOAT, 0, vertices);
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
#endif
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
    
#ifndef IOS_BUILD
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
#else
    std::vector<GLfloat> vertices;
    
    for (i = 0; i < num_segments; i++)
    {
        vertices.push_back( x + cx );
        vertices.push_back( y + cy );

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer( 2, GL_FLOAT, 0, vertices.data());
    glDrawArrays( GL_LINE_LOOP, 0, num_segments);
    glDisableClientState(GL_VERTEX_ARRAY);
#endif
    
    glDisable(GL_BLEND);
}

void circle_fill(float cx, float cy, float r, int num_segments, float red, float green, float blue, float alpha)
{
    float theta = 2 * 3.1415926 / (float)num_segments;
    float c = cosf(theta); // precalculate the sine and cosine
    float s = sinf(theta);
    float t;
    float x = r; // we start at angle = 0
    float y = 0;
    
    glColor4f(red, green, blue, alpha);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef IOS_BUILD
    glBegin(GL_POLYGON);
    for (size_t i = 0; i < num_segments; i++)
    {
        glVertex2f(x + cx, y + cy); // output vertex

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    glEnd();
#else
    std::vector<GLfloat> vertices;
    
    vertices.push_back( 0.0 );
    vertices.push_back( 0.0 );
    
    for( size_t i = 0; i < num_segments; i++)
    {
        vertices.push_back( x + cx );
        vertices.push_back( y + cy );
        
        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer( 2, GL_FLOAT, 0, vertices.data());
    glDrawArrays( GL_TRIANGLE_FAN, 0, static_cast<int>(vertices.size() / 2) );
    glDisableClientState(GL_VERTEX_ARRAY);
#endif
    glDisable(GL_BLEND);
}

void draw_textured_sphere(float x, float y, float radius, int textureID, float roll, float pitch, float yaw)
{
#ifndef IOS_BUILD
    GLUquadricObj *q = gluNewQuadric();         // Create A New Quadratic

    /* Set Up Sphere Mapping */
    glEnable(GL_TEXTURE_2D);                    // Enable Texture Mapping
    glEnable(GL_DEPTH_TEST);                    // Enables Depth Testing
    gluQuadricDrawStyle(q, GL_FILL);            // Generate a filled sphere
    gluQuadricNormals(q, GL_SMOOTH);            // Generate Smooth Normals For The Quad
    gluQuadricTexture(q, GL_TRUE);              // Enable Texture Coords For The Quad
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    glBindTexture(GL_TEXTURE_2D, textureID);

    glPushMatrix();
        glTranslatef(x, y, 0);
        glScalef(radius, radius, 1);
        /* Orient the sphere according to the roll, pitch, and yaw */
        glRotatef(-90,    0, 1, 0);
        glRotatef( 90,    0, 0, 1);
        glRotatef(-roll,  0, 1, 0);
        glRotatef(-yaw,   1, 0, 0);
        glRotatef(-pitch, 0, 0, 1);
        /* Draw the sphere */
        gluSphere(q, 1, 32, 32);
    glPopMatrix();
    gluDeleteQuadric(q);
    glDisable(GL_DEPTH_TEST);          // Disable depth testing
    glDisable(GL_TEXTURE_2D);          // Disable texture mapping
#endif
}
