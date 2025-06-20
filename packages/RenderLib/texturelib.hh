#ifndef _TEXTURELIB_HH_
#define _TEXTURELIB_HH_

#include <string>
#include "opengl_draw.hh"

class tdTexture
{
    public:
        tdTexture(const std::string &);
        tdTexture(void);
        virtual ~tdTexture();
        bool isValid(void);
        void setID(unsigned int);
        unsigned int getID(void);
        std::string getFileName(void);
        int loadBMP(void);
        int loadTGA(void);
        int loadJPG(void);
        int loadS3TC(void);
	void setPixelSpec(int);
    private:
        void computeBytesPerPixel(void);
        bool valid;
        unsigned int id;
        std::string filename;
    public:
        int width;
        int height;
        int pixelspec;
        unsigned int bytesPerPixel;
        unsigned char *data;
        bool convertNPOT;
        bool smooth;
        unsigned int size;
};

#endif
