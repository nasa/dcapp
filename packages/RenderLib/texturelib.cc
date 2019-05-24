#include <string>
#include <GL/gl.h>
#include "texturelib.hh"

extern int imgload(const char *);

tdTexture::tdTexture(const char *filespec) : valid(false), id(-1)
{
    if (filespec)
    {
        this->filename = filespec;
        this->id = imgload(filespec);
        this->valid = true;
    }
}

tdTexture::tdTexture(void) : valid(false), id(-1)
{
    glGenTextures(1, (GLuint *)&(this->id));
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Make sure that byte unpacking (for PixelStream, etc.) is properyly byte aligned
    this->valid = true;
}

tdTexture::~tdTexture()
{
}

bool tdTexture::isValid(void)
{
    return this->valid;
}

unsigned int tdTexture::getID(void)
{
    return this->id;
}

std::string tdTexture::getFileName(void)
{
    return this->filename;
}
