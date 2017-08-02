#ifndef _LOADUTILS_HH_
#define _LOADUTILS_HH_

#include "dc.hh"
#include <vector>

typedef struct
{
	dcTexture textureID;    // changed for easier searching
	std::string m_FullPath;
	std::string	m_Filename;
} textureInfo;

extern dcTexture dcLoadTexture(const char *filename);
extern dcFont dcLoadFont(const std::string &filename, const char *face=0x0, unsigned int basesize=20);
extern float *dcLoadConstant(float fval);
extern int *dcLoadConstant(int ival);
extern char *dcLoadConstant(const char *sval);

size_t getTextureCount( void );
const textureInfo *getTextureInfo( const std::string &fileNameA );
std::string getFullPath(const std::string &fname);
std::string getBasePath( void );
std::string getFontPath( const std::string &fontFileA );

#endif
