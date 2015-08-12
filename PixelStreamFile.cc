#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include "PixelStreamFile.hh"

extern void *LoadShm(key_t);

PixelStreamFile::PixelStreamFile()
:
filename(0x0),
fp(0x0),
shmemkey(0),
shm(0x0),
buffercount(0)
{
}

PixelStreamFile::~PixelStreamFile()
{
    if (this->filename) free(this->filename);
}

int PixelStreamFile::initialize(char *filenamespec, int shmemkeyspec)
{
    if (!filenamespec) return -1;
    this->filename = strdup(filenamespec);
    this->shmemkey = shmemkeyspec;
    this->shm = (PixelStreamShmem *)LoadShm(shmemkeyspec);
    return 0;
}

int PixelStreamFile::update(void)
{
    int updated = 0;
    uint32_t on = 1, off = 0;

    if (!(this->fp))
    {
        if (this->filename) this->fp = fopen(this->filename, "r");
    }
    else if (this->shm)
    {
        if (!(this->shm->writing))
        {
            memcpy(&(this->shm->reading), &on, 4);
            if (this->buffercount != this->shm->buffercount)
            {
                this->buffercount = this->shm->buffercount;
                this->width = this->shm->width;
                this->height = this->shm->height;
                rewind(this->fp);
                size_t nbytes = (size_t)(this->width * this->height * 3);
                this->pixels = realloc(this->pixels, nbytes);
                fread(this->pixels, 1, nbytes, this->fp);
                updated = 1;
            }
            memcpy(&(this->shm->reading), &off, 4);
        }
    }

    return updated;
}

char *PixelStreamFile::getFileName(void)
{
    return this->filename;
}

int PixelStreamFile::getShmemKey(void)
{
    return this->shmemkey;
}
