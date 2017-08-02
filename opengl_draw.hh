#ifndef _OPENGL_DRAW_HH_
#define _OPENGL_DRAW_HH_
#include <vector>

#include "fontlib/fontlib.hh"

extern void graphics_init(void);
extern void reshape(int, int);
extern void init_texture(unsigned int *);
extern void set_texture(unsigned int, int, int, void *);
extern void draw_image(unsigned int, float, float);
extern void get_image_pixel_RGBA(unsigned char[], unsigned int, float, float);
extern int build_font(int, int);
extern void setup_panel(float, float, int, int, float, float, float, float);
extern float get_string_width(flFont *, float, flMonoOption, const char *);
extern void draw_string(float, float, float, float, float, float, float, flFont *, flMonoOption, const char *);
extern void container_start(float, float, float, float, float, float, float);
extern void container_end(void);
extern void rotate_start(float);
extern void rotate_end(void);
extern void translate_start(float, float);
extern void translate_end(void);

extern void circle_outline(float, float, float, int, float, float, float, float, float);
extern void circle_fill(float, float, float, int, float, float, float, float);

void draw_textured_sphere(float x, float y, const std::vector<float > &pointsA, float radius, int texID, float roll, float pitch, float yaw );
void draw_line(const std::vector< float > &pntsA, float linewidth, float red, float green, float blue, float alpha);
void draw_filled_triangles( const std::vector< float >&pntsA, float red, float green, float blue, float alpha);
void draw_quad( const std::vector< float >&pntsA, float red, float green, float blue, float alpha);
void draw_polygon( const std::vector< float >&pntsA, float red, float green, float blue, float alpha);

// convenience functions
void addPoint( std::vector<float> &listA, float xA, float yA );
void addPoint( std::vector<float> &listA, float xA, float yA, float zA );
void addPoint( std::vector<float> &listA, float xA, float yA, float zA, float uA, float vA );

void CheckError( const char *idA );
#endif
