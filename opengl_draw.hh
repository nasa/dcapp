#ifndef _OPENGL_DRAW_HH_
#define _OPENGL_DRAW_HH_

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
extern void line_start(float, float, float, float, float);
extern void line_end(void);
extern void polygon_outline_start(float, float, float, float, float);
extern void polygon_outline_end(void);
extern void polygon_fill_start(float, float, float, float);
extern void polygon_fill_end(void);
extern void gfx_vertex(float, float);
extern void rectangle_outline(float, float, float, float, float, float, float, float, float, float, float, float, float);
extern void rectangle_fill(float, float, float, float, float, float, float, float, float, float, float, float);
extern void circle_outline(float, float, float, int, float, float, float, float, float);
extern void circle_fill(float, float, float, int, float, float, float, float);
extern void draw_textured_sphere(float, float, float, int, float, float, float);

#endif
