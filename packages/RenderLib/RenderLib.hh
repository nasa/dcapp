#ifndef _RENDERLIB_HH_
#define _RENDERLIB_HH_

#include <string>
#include <vector>
#include "fontlib.hh"
#include "texturelib.hh"

extern void init_window(void);
extern void reshape_window(int, int);
extern void setup_panel(float, float, float, float, float, float);
extern void create_and_load_glyph(unsigned int *, void *);
extern void create_texture(tdTexture *);
extern void load_texture(tdTexture *);
extern void draw_string(float, float, float, float, float, float, float, tdFont *, flMonoOption, std::string);
extern void draw_glyph(unsigned int, float, float, float, float);
extern void draw_image(tdTexture *, float, float);
extern void get_image_pixel_RGBA(unsigned char[], unsigned int, float, float);
extern void container_start(float, float, float, float, float, float, float);
extern void container_end(void);
extern void rotate_start(float);
extern void rotate_end(void);
//extern void translate_start(float, float);
//extern void translate_end(void);
extern void draw_line(const std::vector<float> &, float, float, float, float, float);
extern void draw_polygon(const std::vector<float> &, float, float, float, float);
extern void draw_filled_triangles(const std::vector<float> &, float, float, float, float);
extern void draw_quad(const std::vector<float> &, float, float, float, float);
extern void circle_outline(float, float, float, int, float, float, float, float, float);
extern void circle_fill(float, float, float, int, float, float, float, float);
extern void draw_textured_sphere(float, float, const std::vector<float> &, float, tdTexture *, float, float, float);

#endif