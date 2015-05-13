#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glu.h>
#include "opengl_draw.hh"
#ifdef UseFTGL
#include <FTGL/ftgl.h>
#else
#include "fontlib.hh"
#endif

#define FONTSIZE 20


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
    glOrtho(0, x, 0, y, near, far);
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

float get_string_width(void *fontID, float charH, flMonoOption mono, char *string)
{
    if (!fontID || !string) return 0;
#ifdef UseFTGL
    return ftglGetFontAdvance(fontID, string)*charH/FONTSIZE;
#else
    return flGetFontAdvance((flFont *)fontID, mono, string)*charH/FONTSIZE;
#endif
}

void draw_string(float xpos, float ypos, float charH, float red, float green, float blue, float alpha, void *fontID, flMonoOption mono, char *string)
{
    glColor4f(red, green, blue, alpha);
    glPushMatrix();
        glEnable(GL_TEXTURE_2D);                                       // Enable Texture Mapping
        glEnable(GL_BLEND);                                            // Blend the transparent part with the background
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifdef UseFTGL
        glTranslatef(xpos, ypos-(ftglGetFontDescender(fontID)*charH/FONTSIZE), 0);
        glScalef(charH/FONTSIZE, charH/FONTSIZE, 0);
        ftglRenderFont(fontID, string, FTGL_RENDER_ALL);
#else
        glTranslatef(xpos, ypos-(flGetFontDescender((flFont *)fontID)*charH/FONTSIZE), 0);
        glScalef(charH/FONTSIZE, charH/FONTSIZE, 0);
        flRenderFont((flFont *)fontID, mono, string);
#endif
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
    glPushMatrix();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(linewidth);
        glColor4f(red, green, blue, alpha);
        glBegin(GL_LINE_STRIP);
}

void line_end(void)
{
        glEnd();
    glPopMatrix();
}

void polygon_outline_start(float linewidth, float red, float green, float blue, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glLineWidth(linewidth);
    glBegin(GL_LINE_LOOP);
}

void polygon_outline_end(void)
{
    glEnd();
}

void polygon_fill_start(float red, float green, float blue, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glBegin(GL_POLYGON);
}

void polygon_fill_end(void)
{
    glEnd();
}

void gfx_vertex(float x, float y)
{
    glVertex2f(x, y);
}

void rectangle_outline(float linewidth, float red, float green, float blue, float alpha, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glLineWidth(linewidth);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
    glEnd();
}

void rectangle_fill(float red, float green, float blue, float alpha, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
    glEnd();
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

void draw_textured_sphere(float x, float y, float z, float radius, int textureID, float roll, float pitch, float yaw)
{
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
#if 0
        glRotatef(-roll    , 0, 1, 0);    /* R */
        glRotatef( yaw  +90, 0, 0, 1);    /* Y */
        glRotatef(-pitch-90, 1, 0, 0);    /* P */
#else
        glRotatef(-90,    0, 1, 0);
        glRotatef( 90,    0, 0, 1);
        glRotatef(-roll,  0, 1, 0);
        glRotatef(-yaw,   1, 0, 0);
        glRotatef(-pitch, 0, 0, 1);
#endif
        gluSphere(q, 1, 32, 32);       // Draw sphere
    glPopMatrix();
    gluDeleteQuadric(q);
    glDisable(GL_DEPTH_TEST);          // Disable depth testing
    glDisable(GL_TEXTURE_2D);          // Disable texture mapping
}
