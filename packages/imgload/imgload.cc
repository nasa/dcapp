#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <vector>
#include <iosfwd>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "basicutils/msg.hh"
#include "imgload_internal.hh"

extern unsigned int LoadTGA(const char *, ImageStruct *);
extern int LoadBMP(const char *, ImageStruct *);
extern unsigned int LoadJPG(const char *, ImageStruct *);
extern int createTextureFromImage(ImageStruct *);

static const std::string FindExtension(const std::string &filename)
{
    if ( filename.empty() )
    {
        error_msg("No filename specified");
        return std::string("");
    }

    auto end = filename.find_last_of('.');
    if (end == std::string::npos )
    {
        error_msg("No detectable filename extension for file " << filename);
        return 0x0;
    }

    auto string1L = filename.substr( end+1 );
    std::string string2L;
    
    std::transform( string2L.begin(), string2L.end(), std::back_inserter( string1L ), tolower );
    
    return filename.substr(end+1);
}


bool DoesFileExist( const std::string &nameA )
{
	FILE *file = fopen( nameA.c_str(), "r"); // Open the file

	if( file != nullptr )
	{
		fclose( file );
		return true;
	}
	
	return false;
	
//	std::ifstream fileL( nameA.c_str());
//	return fileL.good();
}

int imgload(const std::string &filename )
{
    ImageStruct image;
    int texture;

    auto extension = FindExtension( filename );
    
    if ( extension.empty() )
    {
        return (-1);
    }
    else if ( extension == "bmp" )
    {
        if (LoadBMP(filename.c_str(), &image) == -1)
        {
            error_msg("LoadBMP returned with error for file " << filename);
            return (-1);
        }
    }
    else if ( extension == "tga" )
    {
        if (LoadTGA(filename.c_str(), &image))
        {
            error_msg("LoadTGA returned with error for file " << filename);
            return (-1);
        }
    }
    else if ( extension == "jpg" || extension == "jpeg" )
    {
        if (LoadJPG(filename.c_str(), &image))
        {
            error_msg("LoadJPG returned with error for file " << filename);
            return (-1);
        }
    }
    else
    {
        error_msg("Unsupported extension for file " << filename << ": " << extension);
        return (-1);
    }

    texture = createTextureFromImage(&image);
    if (image.data ) free(image.data);
    return texture;
}

std::string LoadFileAsOne( const std::string &filename )
{
	std::ifstream inFileL( filename, std::ifstream::in | std::ofstream::binary );

	auto ss = std::ostringstream{};
	ss << inFileL.rdbuf();
	return ss.str();
}

bool CreateDir( const std::string &pathA )
{
	if( opendir( pathA.c_str() ) )
	   return true;
	   
   auto indexL = pathA.find_last_of("/");
   if( indexL != std::string::npos )
   {
	   auto dirL = pathA.substr( 0, indexL );
	   if( CreateDir( dirL ) )
	   {
		   const int dir_err = mkdir( pathA.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		   if (-1 == dir_err)
		   {
			   return false;
		   }
	   }
	   else
		   return false;
	}
	
	return true;
}

void SaveFileData( const std::string &pathA, const std::string &fileDataA )
{
	auto indexL = pathA.find_last_of("/");
	if( indexL != std::string::npos )
	{
		auto dirL = pathA.substr( 0, indexL );
		CreateDir( dirL );
	}
	
	std::ofstream outFileL( pathA, std::ofstream::out | std::ofstream::binary );
	
//	outFileL << fileDataA;
	outFileL.write( fileDataA.data(), fileDataA.size() );
	
	if( outFileL.fail())
		std::cout << "Failed to write " << pathA << std::endl;
	
	outFileL.close();
}

bool LoadImageFile(const std::string &filename, std::vector<unsigned char> &imageDataA, uint32_t &widthA, uint32_t &heightA, uint32_t &pixelSpecA )
{
    ImageStruct image;
	std::cout << "LoadImageFile " << filename << std::endl;
    auto extension = FindExtension( filename );
    
    if ( extension.empty() )
    {
        return false;
    }
    else if ( extension == "bmp" )
    {
        if (LoadBMP(filename.c_str(), &image) == -1)
        {
            error_msg("LoadBMP returned with error for file " << filename);
            return false;
        }
    }
    else if ( extension == "tga" )
    {
        if (LoadTGA(filename.c_str(), &image))
        {
            error_msg("LoadTGA returned with error for file " << filename);
            return false;
        }
    }
    else if ( extension == "jpg" || extension == "jpeg" )
    {
        if (LoadJPG(filename.c_str(), &image))
        {
            error_msg("LoadJPG returned with error for file " << filename);
            return false;
        }
    }
    else
    {
        error_msg("Unsupported extension for file " << filename << ": " << extension);
        return false;
    }
	
	int sizeL = 0;
	
	switch (image.pixelspec)
	{
		case PixelLuminance:
			sizeL = image.width * image.height;
			break;
		case PixelLuminanceAlpha:
			sizeL = image.width * image.height * 2;
			break;
		case PixelRGB:
			sizeL = image.width * image.height * 3;
			break;
		case PixelRGBA:
			sizeL = image.width * image.height * 4;
			break;
		default:
			sizeL = image.width * image.height * 3;
	}
	
	auto ptrL = image.data;
	for( size_t iL=0; iL<sizeL; iL++, ++ptrL )
		imageDataA.push_back( *ptrL );
	
	widthA		= static_cast<uint32_t>(image.width);
	heightA		= static_cast<uint32_t>(image.height);
	pixelSpecA	= static_cast<uint32_t>(image.pixelspec);
	
    return true;
}
