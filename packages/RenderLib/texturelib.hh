#ifndef _TEXTURELIB_HH_
#define _TEXTURELIB_HH_

#include <string>

typedef enum { PixelUnknown, PixelLuminance, PixelLuminanceAlpha, PixelRGB, PixelRGBA } PixelSpec;

class tdTexture
{
    public:
        tdTexture(const char *);
        tdTexture(void);
        virtual ~tdTexture();
        bool isValid(void);
        void setID(unsigned int);
        unsigned int getID(void);
        std::string getFileName(void);
        int loadBMP(void);
        int loadTGA(void);
        int loadJPG(void);
    private:
        bool valid;
        unsigned int id;
        std::string filename;
    public:
        int width;
        int height;
        PixelSpec pixelspec;
        unsigned int bytesPerPixel;
        unsigned char *data;
};

#endif
