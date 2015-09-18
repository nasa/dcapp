#include <stdlib.h>
#include <string.h>
#include <list>
#include "imgload/imgload.hh"

class dcTexture
{
    public:
        dcTexture();
        virtual ~dcTexture();

        void create(const char *);
        char *getFileName(void);
        unsigned int getID(void);

    private:
        char *filename;
        unsigned int id;
};

static std::list<dcTexture *> mytextures;

int LoadTexture(const char *filename)
{
    std::list<dcTexture *>::iterator dct;
    for (dct = mytextures.begin(); dct != mytextures.end(); dct++)
    {
        if (!strcmp((*dct)->getFileName(), filename)) return (*dct)->getID();
    }

    dcTexture *newtexture = new dcTexture;
    newtexture->create(filename);
    mytextures.push_back(newtexture);
    return newtexture->getID();
}

dcTexture::dcTexture()
:
filename(0x0),
id(-1)
{
}

dcTexture::~dcTexture()
{
    if (this->filename) free(this->filename);
}

void dcTexture::create(const char *infile)
{
    if (!infile) return;
    this->filename = strdup(infile);
    this->id = imgload(this->filename);
}

char * dcTexture::getFileName(void)
{
    return this->filename;
}

unsigned int dcTexture::getID(void)
{
    return this->id;
}
