#include <string>
#include "basicutils/msg.hh"
#include "RenderLib.hh"
#include "texturelib.hh"
#include "imgload.hh"

extern int LoadBMP(const char *, ImageStruct *);
extern unsigned int LoadTGA(const char *, ImageStruct *);
extern unsigned int LoadJPG(const char *, ImageStruct *);
extern int createTextureFromImage(ImageStruct *);

tdTexture::tdTexture(const char *filespec) : valid(false), id(-1)
{
    if (filespec)
    {
        this->filename = filespec;

        size_t end = this->filename.rfind('.');
        if (end == std::string::npos) warning_msg("No detectable filename extension for file " << filespec);
        else
        {
            ImageStruct image;
            bool success = false;

            std::string suffix = this->filename.substr(end + 1);
            std::locale loc;
            for (size_t i=0; i<suffix.length(); i++) suffix[i] = std::toupper(suffix[i], loc);

            if (suffix == "BMP")
            {
                if (LoadBMP(filespec, &image) != -1) success = true;
                else error_msg("LoadBMP returned with error for file " << filespec);
            }
            else if (suffix == "TGA")
            {
                if (!LoadTGA(filespec, &image)) success = true;
                else error_msg("LoadTGA returned with error for file " << filespec);
            }
            else if (suffix == "JPG" || suffix == "JPEG")
            {
                if (!LoadJPG(filespec, &image)) success = true;
                else error_msg("LoadJPG returned with error for file " << filespec);
            }
            else
            {
                warning_msg("Unsupported extension for file " << filespec << ": " << suffix);
            }

            if (success)
            {
                this->id = createTextureFromImage(&image);
                this->valid = true;
            }

            if (image.data) free(image.data);
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
