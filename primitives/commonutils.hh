#ifndef _POINTS_HH_
#define _POINTS_HH_

#include <vector>
#include "RenderLib/fontlib.hh"
#include "RenderLib/texturelib.hh"

extern tdTexture *tdLoadTexture(const char *filename);
extern tdFont *tdLoadFont(const char *filename, const char *face=0x0, unsigned int basesize=20);
extern void addPoint(std::vector<float> &, float, float);
extern void addPoint(std::vector<float> &, float, float, float);
extern void addPoint(std::vector<float> &, float, float, float, float, float);

#endif
