#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <list>
#include <memory>
#include <strings.h>
#include "basicutils/msg.hh"
#include "imgload/imgload.hh"
#include "fontlib/fontlib.hh"
#include "dc.hh"
#include "nodes.hh"
#include "TaraDraw/TaraDraw.hh"

extern appdata AppData;

#ifdef IOS_BUILD
#include <CoreFoundation/CoreFoundation.h>
#endif

bool RequestFile( DCA_Comm::Commands commandA, const std::string &fileNameA );

using namespace std;

static list<textureInfo> textures;
static list<dcFont> fonts;
static map<std::string, std::string>	fontMap;
static list<float> floatConstants;
static list<int> integerConstants;
static list<char *> stringConstants;

size_t getTextureCount( void )
{
	return textures.size();
}

const textureInfo *getTextureInfo( const std::string &fileNameA )
{
	for( const auto &pTexL : textures )
	{
		if( pTexL.m_Filename == fileNameA )
		{
			return &pTexL;
		}
	}
	
	return nullptr;
}

size_t getFontCount( void )
{
	return fonts.size();
}

std::string getFontPath( const std::string &fontFileA )
{
	return fontMap[fontFileA];
}

std::string getFullPath(const std::string &fname)
{
#ifndef IOS_BUILD
    if (fname.empty()) return fname;

    std::shared_ptr<char> longname(new char[PATH_MAX], std::default_delete<char[]>());
    
	if (!realpath(fname.c_str(), longname.get() ))
	{
	    return std::string("");
	}
	std::string shortname( longname.get() );
	return shortname;
#else
	return getBasePath() + "/" + fname;
#endif
}

std::string getBasePath( void )
{
#ifndef IOS_BUILD
	return std::string( "./" );
#else
	return AppData.documentsFolder;
#endif
}

dcTexture dcLoadTexture(const char *filename)
{
	const auto fullpath = getFullPath(filename);
	
	if ( fullpath.empty() )
	{
		error_msg("Unable to locate texture file at " << filename);
		return -1;
	}
	
	for( const auto &pTexL : textures )
	{
		if( pTexL.m_FullPath == fullpath )
		{
			return pTexL.textureID;
		}
	}
	
	
#ifdef IOS_BUILD
	if( DoesFileExist( fullpath ) == false || AppData.forceDownload )
	{
		if( RequestFile( DCA_Comm::Commands::FetchTextureMap, filename ) == false )
			return -1;
	}
#endif
	
    textureInfo newtexture;
    newtexture.textureID	= imgload(fullpath);
    newtexture.m_FullPath	= fullpath;
	newtexture.m_Filename	= filename;

    textures.push_back(newtexture);
    return newtexture.textureID;
}

dcFont dcLoadFont(const std::string &filename, const char *face, unsigned int basesize)
{
    auto fullpath = getFullPath(filename);

    if (fullpath.empty() )
    {
        error_msg("Unable to locate font file at " << filename);
        return nullptr;
    }
	
    for (list<dcFont>::iterator item = fonts.begin(); item != fonts.end(); item++)
    {
        if (((*item)->getFileName() == fullpath) && ((*item)->getBaseSize() == basesize))
        {
            if (!face && (*item)->getFaceName().empty())
            {
                return *item;
            }
            else if (face && !((*item)->getFaceName().empty()))
            {
                if ((*item)->getFaceName() == face)
                {
                    return *item;
                }
            }
        }
    }
	
#ifdef IOS_BUILD
	if( DoesFileExist( fullpath ) == false || AppData.forceDownload )
	{
		if( RequestFile( DCA_Comm::Commands::FetchFont, filename ) == false )
			return nullptr;
	}
#endif
	
	fontMap[filename] = fullpath;
	
    dcFont id = new flFont(fullpath.c_str(), face, basesize);
    if (!(id->isValid())) error_msg("Could not load font " << filename);
    fonts.push_back(id);
    return id;
}

float *dcLoadConstant(float fval)
{
    list<float>::iterator fc;
    for (fc = floatConstants.begin(); fc != floatConstants.end(); fc++)
    {
        if (*fc == fval) return &(*fc);
    }
    floatConstants.push_back(fval);
    return &(floatConstants.back());
}

int *dcLoadConstant(int ival)
{
    list<int>::iterator ic;
    for (ic = integerConstants.begin(); ic != integerConstants.end(); ic++)
    {
        if (*ic == ival) return &(*ic);
    }
    integerConstants.push_back(ival);
    return &(integerConstants.back());
}

char *dcLoadConstant(const char *sval)
{
    list<char *>::iterator sc;
    for (sc = stringConstants.begin(); sc != stringConstants.end(); sc++)
    {
        if (!strcmp(*sc, sval)) return *sc;
    }
    stringConstants.push_back(strdup(sval));
    return stringConstants.back();
}
