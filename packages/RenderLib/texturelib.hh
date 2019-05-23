#ifndef _TEXTURELIB_HH_
#define _TEXTURELIB_HH_

extern int imgload(const char *);
extern unsigned int init_texture(void);

class tdTexture
{
    public:
        tdTexture(const char *filespec) : valid(false), id(-1)
        {
            if (filespec)
            {
                this->filename = filespec;
                this->id = imgload(filespec);
                this->valid = true;
            }
        };
        tdTexture(void) : valid(false), id(-1)
        {
            this->id = init_texture();
            this->valid = true;
        };
        virtual ~tdTexture() {};
        bool isValid(void) { return this->valid; };
        unsigned int getID(void) { return this->id; };
        std::string getFileName(void) { return this->filename; };
    private:
        bool valid;
        unsigned int id;
        std::string filename;
};

#endif