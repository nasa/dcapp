#ifndef _TEXTURELIB_HH_
#define _TEXTURELIB_HH_

#include <string>

class tdTexture
{
    public:
        tdTexture(const char *);
        tdTexture(void);
        virtual ~tdTexture();
        bool isValid(void);
        unsigned int getID(void);
        std::string getFileName(void);
    private:
        bool valid;
        unsigned int id;
        std::string filename;
};

#endif
