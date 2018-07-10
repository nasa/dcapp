#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/shm.h>
#include "PixelStreamFile.hh"
#include "basicutils/msg.hh"

// This is a safe size.  The shmget call will fail if it's too small or too large.  It only needs
// to be large enough to house the elements of the PixelStreamShmem structure defined below.
#define SHM_SIZE 1024

PixelStreamFile::PixelStreamFile()
:
filename(0x0),
fp(0x0),
shmemkey(0),
shm(0x0),
buffercount(0)
{
    this->protocol = PixelStreamFileProtocol;
}

PixelStreamFile::~PixelStreamFile()
{
    if (this->shm > (void *)0) shmdt(this->shm);
    if (this->fp) fclose(this->fp);
    if (this->filename) free(this->filename);
}

bool PixelStreamFile::operator == (const PixelStreamFile &that)
{
    if (!strcmp(this->filename, that.filename) && this->shmemkey == that.shmemkey) return true;
    else return false;
}

bool PixelStreamFile::operator != (const PixelStreamFile &that)
{
    return !(*this == that);
}

int PixelStreamFile::genericInitialize(const char *filenamespec, int shmemkeyspec)
{
    if (!filenamespec) return -1;
    this->filename = strdup(filenamespec);
    this->shmemkey = shmemkeyspec;

    int shmid = shmget(shmemkeyspec, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        error_msg("Unable to get shared memory identifier: " << strerror(errno));
        return -1;
    }
    this->shm = (PixelStreamShmem *)shmat(shmid, 0x0, 0);
    if (this->shm <= (void *)0)
    {
        error_msg("Unable to attach to shared memory: " << strerror(errno));
        return -1;
    }

    return 0;
}

int PixelStreamFile::readerInitialize(const char *filenamespec, int shmemkeyspec)
{
    return genericInitialize(filenamespec, shmemkeyspec);
}

int PixelStreamFile::writerInitialize(const char *filenamespec, int shmemkeyspec)
{
    if (genericInitialize(filenamespec, shmemkeyspec)) return -1;

    memset(this->shm, 0, SHM_SIZE);

    if (this->filename) this->fp = fopen(this->filename, "w");
    if (!(this->fp))
    {
        error_msg("PixelStream: Couldn't open stream file " << this->filename);
        return -1;
    }

    return 0;
}

int PixelStreamFile::reader(void)
{
    int updated = 0;
    uint32_t on = 1, off = 0;

    connected = false;

    if (!(this->fp))
    {
        if (this->filename) this->fp = fopen(this->filename, "r");
    }
    else if (this->shm > (void *)0)
    {
        connected = true;
        if (!(this->shm->writing))
        {
            memcpy(&(this->shm->bufferrequested), &on, 4);
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

bool PixelStreamFile::writeRequested(void)
{
    if (this->pixels && this->shm->bufferrequested) return true;
    else return false;
}

int PixelStreamFile::writer(void)
{
    uint32_t on = 1, off = 0;

    if (!(this->pixels)) return 0;

    if (!(this->shm->reading))
    {
            memcpy(&(this->shm->writing), &on, 4);
            rewind(this->fp);
            fwrite(this->pixels, 1, (size_t)(this->width * this->height * 3), this->fp);
            memcpy(&(this->shm->buffercount), &(this->buffercount), 8);
            memcpy(&(this->shm->width), &(this->width), 4);
            memcpy(&(this->shm->height), &(this->height), 4);
            fflush(this->fp);
            memcpy(&(this->shm->writing), &off, 4);
            memcpy(&(this->shm->bufferrequested), &off, 4);
    }
    (this->buffercount)++;

    return 1;
}

char *PixelStreamFile::getFileName(void)
{
    return this->filename;
}

int PixelStreamFile::getShmemKey(void)
{
    return this->shmemkey;
}
