#include <string>
#include "basicutils/msg.hh"
#include "RenderLib.hh"
#include "texturelib.hh"

extern void createTextureFromImage(tdTexture *);

tdTexture::tdTexture(const char *filespec) : valid(false), id(-1), pixelspec(PixelRGB), data(0x0)
{
    if (filespec)
    {
        this->filename = filespec;

        size_t end = this->filename.rfind('.');
        if (end == std::string::npos) warning_msg("No detectable filename extension for file " << filespec);
        else
        {
            bool success = false;

            std::string suffix = this->filename.substr(end + 1);
            std::locale loc;
            for (size_t i=0; i<suffix.length(); i++) suffix[i] = std::toupper(suffix[i], loc);

            if (suffix == "BMP")
            {
                if (!this->loadBMP()) success = true;
            }
            else if (suffix == "TGA")
            {
                if (!this->loadTGA()) success = true;
            }
            else if (suffix == "JPG" || suffix == "JPEG")
            {
                if (!this->loadJPG()) success = true;
            }
            else
            {
                warning_msg("Unsupported extension for file " << filespec << ": " << suffix);
            }

            if (success)
            {
                create_texture(this);
                createTextureFromImage(this);
                this->valid = true;
            }

            if (this->data) free(this->data);
        }
    }
}

tdTexture::tdTexture(void) : valid(false), id(-1)
{
    create_texture(this);
    this->valid = true;
}

tdTexture::~tdTexture()
{
}

bool tdTexture::isValid(void)
{
    return this->valid;
}

void tdTexture::setID(unsigned int val)
{
    this->id = val;
}

unsigned int tdTexture::getID(void)
{
    return this->id;
}

std::string tdTexture::getFileName(void)
{
    return this->filename;
}
