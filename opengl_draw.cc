#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <assert.h>
#include <iostream>

#ifndef IOS_BUILD
#include <GL/glu.h>
#else
#include <OpenGLES/ES1/glext.h>
#endif
#include "fontlib/fontlib.hh"

void CheckError( const char *idA )
{
    auto errorL = glGetError();
    if( errorL )
        std::cout << "Error(" << idA << ") = " << errorL << std::endl;
}

// convenience functions
void addPoint( std::vector<float> &listA, float xA, float yA )
{
	listA.push_back( xA );
	listA.push_back( yA );
}

void addPoint( std::vector<float> &listA, float xA, float yA, float zA )
{
	listA.push_back( xA );
	listA.push_back( yA );
	listA.push_back( zA );
}

void addPoint( std::vector<float> &listA, float xA, float yA, float zA, float uA, float vA )
{
	listA.push_back( xA );
	listA.push_back( yA );
	listA.push_back( zA );
	listA.push_back( uA );
	listA.push_back( vA );
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
	if( width ==  0 || height == 0 )
		return;
//
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

void draw_image(unsigned int textureID, float w, float h)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, textureID);
    glColor4f(1, 1, 1, 1);
	
    const GLfloat vertices[8]   = { 0,h, 0,0, w,h, w,0 };
    const GLfloat uvs[8]        = { 0,1, 0,0, 1,1, 1,0 };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer( 2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture( GL_TEXTURE_2D, 0 );
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

void draw_line(const std::vector< float > &pntsA, float linewidth, float red, float green, float blue, float alpha)
{
	glPushMatrix();
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth( linewidth );
	glColor4f( red, green, blue, alpha );
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer( 2, GL_FLOAT, 0, pntsA.data() );
	glDrawArrays( GL_LINE_STRIP, 0, static_cast<int>(pntsA.size()/2) );
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
	
	glPopMatrix();
}

void draw_filled_triangles( const std::vector< float >&pntsA, float red, float green, float blue, float alpha)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(red, green, blue, alpha);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer( 2, GL_FLOAT, 0, pntsA.data() );
	glDrawArrays( GL_TRIANGLES, 0, static_cast<int>(pntsA.size() / 2) );
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
}

void draw_quad( const std::vector< float >&pntsA, float red, float green, float blue, float alpha)
{
	assert( pntsA.size() == 8 );
	std::vector< float > localPointsL;
//	localPointsL.reserve(8);
	
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
	glVertexPointer( 2, GL_FLOAT, 0, localPointsL.data() );
	glDrawArrays( GL_TRIANGLES, 0, static_cast<int>(localPointsL.size() / 2) );
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
}

void draw_polygon( const std::vector< float >&pntsA, float red, float green, float blue, float alpha)
{
	std::vector< float > localPointsL;

//	localPointsL.push_back( pntsA[0] ); // x0
//	localPointsL.push_back( pntsA[1] ); // y0
	for( size_t iL=0; iL< pntsA.size()-1; iL+=2 )
	{
		localPointsL.push_back( pntsA[iL] );
		localPointsL.push_back( pntsA[iL+1] );
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glColor4f(red, green, blue, alpha);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer( 2, GL_FLOAT, 0, localPointsL.data() );
	glDrawArrays( GL_TRIANGLE_FAN, 0, static_cast<int>(localPointsL.size() / 2) );
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

    std::vector<GLfloat> vertices;
    
//	vertices.push_back( 0.0 );
//	vertices.push_back( 0.0 );
	
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

    glDisable(GL_BLEND);
}

void draw_textured_sphere(float x, float y, const std::vector<float > &pointsA, float radiusA, int textureID, float roll, float pitch, float yaw)
{
//	glEnable(GL_DEPTH_TEST);                    // Enables Depth Testing
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_CULL_FACE );
	glBindTexture(GL_TEXTURE_2D, textureID);
	glColor4f( 1.0, 1.0, 1.0, 1.0 );
	glPushMatrix();
		glTranslatef(x, y, 0);
		glScalef(radiusA, radiusA, 1.0 );
		/* Orient the sphere according to the roll, pitch, and yaw */
		glRotatef( 180, 0, 0, 1 );
	
		glRotatef(-roll,  0, 1, 0);
		glRotatef(-yaw,   1, 0, 0);
		glRotatef(-pitch, 0, 0, 1);
		/* Draw the sphere */

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer( 2, GL_FLOAT, sizeof(float) * 5, pointsA.data() + 3 );
		glVertexPointer( 3, GL_FLOAT, sizeof(float) * 5, pointsA.data() );
	
		glDrawArrays( GL_TRIANGLES, 0, static_cast<int>(pointsA.size() / 5) );
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPopMatrix();
	
	glBindTexture( GL_TEXTURE_2D, 0 );
	glDisable(GL_TEXTURE_2D );
	glDisable(GL_CULL_FACE);
//	glDisable(GL_DEPTH_TEST);                    // disables Depth Testing
}
