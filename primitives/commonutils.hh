#ifndef _POINTS_HH_
#define _POINTS_HH_

#include <string>
#include <vector>
#include "RenderLib/fontlib.hh"
#include "RenderLib/texturelib.hh"

extern tdTexture *tdLoadTexture(const std::string &);
extern tdFont *tdLoadFont(const std::string &, const std::string &, unsigned int = 20);
extern void addPoint(std::vector<float> &, float, float);
extern void addPoint(std::vector<float> &, float, float, float);
extern void addPoint(std::vector<float> &, float, float, float, float, float);

#endif
