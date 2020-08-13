#ifndef _POINTS_HH_
#define _POINTS_HH_

#include <string>
#include <vector>
#include "RenderLib/fontlib.hh"
#include "RenderLib/texturelib.hh"

extern tdTexture *tdLoadTexture(std::string filename);
extern tdFont *tdLoadFont(std::string &filename, std::string &face, unsigned int basesize=20);
extern void addPoint(std::vector<float> &, float, float);
extern void addPoint(std::vector<float> &, float, float, float);
extern void addPoint(std::vector<float> &, float, float, float, float, float);

#endif
