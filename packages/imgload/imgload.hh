#ifndef _IMGLOAD_HH_
#define _IMGLOAD_HH_
#include <string>
#include <vector>

#include "imgload_internal.hh"

extern int imgload(const std::string &fileNameA );

bool		LoadImageFile	(const std::string &filename, std::vector<unsigned char> &, uint32_t &widthA, uint32_t &heightA, uint32_t &pixelSpecA );
std::string LoadFileAsOne	( const std::string &filename );
void		SaveFileData	( const std::string &pathA, const std::string &fileDataA );
bool		DoesFileExist	( const std::string &nameA );

#endif
