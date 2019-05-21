#ifndef _OPENGL_DRAW_HH_
#define _OPENGL_DRAW_HH_

#include <vector>
#include "fontlib.hh"
#include "RenderLib_internal.hh"

extern void init_window(void);
extern void reshape_window(int, int);
extern void setup_panel(float, float, int, int, float, float, float, float);
extern void init_texture(unsigned int *);
extern void set_texture(unsigned int, int, int, void *);
extern void draw_image(unsigned int, float, float);
extern void get_image_pixel_RGBA(unsigned char[], unsigned int, float, float);
extern float get_string_width(flFont *, float, flMonoOption, const char *);
extern void draw_string(float, float, float, float, float, float, float, flFont *, flMonoOption, const char *);
extern void container_start(float, float, float, float, float, float, float);
extern void container_end(void);
extern void rotate_start(float);
extern void rotate_end(void);
extern void translate_start(float, float);
extern void translate_end(void);
extern void draw_line(const std::vector<float> &, float, float, float, float, float);
extern void draw_polygon(const std::vector<float> &, float, float, float, float);
extern void draw_filled_triangles(const std::vector<float> &, float, float, float, float);
extern void draw_quad(const std::vector<float> &, float, float, float, float);
extern void circle_outline(float, float, float, int, float, float, float, float, float);
extern void circle_fill(float, float, float, int, float, float, float, float);
extern void draw_textured_sphere(float, float, const std::vector<float> &, float, int, float, float, float);

extern void addPoint(std::vector<float> &, float, float);
extern void addPoint(std::vector<float> &, float, float, float);
extern void addPoint(std::vector<float> &, float, float, float, float, float);

extern dcTexture dcLoadTexture(const char *filename);
extern dcFont dcLoadFont(const char *filename, const char *face=0x0, unsigned int basesize=20);

#endif
