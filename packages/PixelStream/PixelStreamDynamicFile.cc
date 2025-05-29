#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/shm.h>
#include "PixelStreamDynamicFile.hh"
#include "basicutils/msg.hh"
#include "basicutils/pathinfo.hh"

// This is a safe size.  The shmget call will fail if it's too small or too large.  It only needs
// to be large enough to house the elements of the PixelStreamShmem structure defined below.
#define SHM_SIZE 1024

PixelStreamDynamicFile::PixelStreamDynamicFile() : shm(0x0), buffercount(0)
{
    this->protocol = PixelStreamDynamicFileProtocol;
}

PixelStreamDynamicFile::~PixelStreamDynamicFile()
{
    if (this->shm) shmdt(this->shm);
}

bool PixelStreamDynamicFile::operator == (const PixelStreamDynamicFile &that)
{
    if (this->keyfilename == that.keyfilename) return true;
    else return false;
}

bool PixelStreamDynamicFile::operator != (const PixelStreamDynamicFile &that)
{
    return !(*this == that);
}

int PixelStreamDynamicFile::genericInitialize(const std::string &filenamespec)
{
    if (filenamespec.empty()) return -1;
    this->keyfilename = filenamespec;

    // we are deferring retrieving the shared memory segment until the file
    // we are sharing exists, this is required by the "ftok" function which
    // we use to generate the shared memory key

    return 0;
}

int PixelStreamDynamicFile::readerInitialize(const std::string &filenamespec)
{
    return genericInitialize(filenamespec);
}

int PixelStreamDynamicFile::writerInitialize(const std::string &filenamespec)
{
    if (genericInitialize(filenamespec)) return -1;

    memset(this->shm, 0, SHM_SIZE);
    return 0;
}

int PixelStreamDynamicFile::reader(void)
{
    int updated = 0;
    uint32_t on = 1, off = 0;

    // NOTE (JHH): Unlike the regular file protocol, here we are opening & closing the file every frame.
    //             There does not seem to be a reliable way to ensure that 2 processes will see updates
    //             to a file they both hold open. From a performance standpoint, the overhead is practically
    //             neglible considering we are reading an uncompressed frame from disk!

    connected = false;
    if (this->shm)
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
                FILE *fpTemp = fopen(this->keyfilename.c_str(), "r");
                rewind(fpTemp);
                size_t nbytes = (size_t)(this->width * this->height * 4);
                this->pixels = realloc(this->pixels, nbytes);
                fread(this->pixels, 1, nbytes, fpTemp);
                updated = 1;
                fclose(fpTemp);
            }
            memcpy(&(this->shm->reading), &off, 4);
        }
    }
    else
    {

        PathInfo mypath(this->keyfilename);
        if(mypath.isValid()) // file exist
        {
            this->keyfilename = mypath.getFullPath();

            key_t key= ftok(this->keyfilename.c_str(), 'R'); // generate a unique key for shared memory (in practice)

            int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0777);
            if (shmid < 0)
            {
                error_msg("Unable to get shared memory identifier: " << strerror(errno));
                return 0;
            }
            this->shm = (PixelStreamDynamicFileShmem *)shmat(shmid, 0x0, 0);
            if (!(this->shm))
            {
                error_msg("Unable to attach to shared memory: " << strerror(errno));
                return 0;
            }
        }
    }

    return updated;
}

bool PixelStreamDynamicFile::writeRequested(void)
{
    if (this->pixels && this->shm->bufferrequested) return true;
    else return false;
}

int PixelStreamDynamicFile::writer(void)
{
    uint32_t on = 1, off = 0;

    if (!(this->pixels)) return 0;

    if (!(this->shm->reading))
    {
            memcpy(&(this->shm->writing), &on, 4);
            FILE *fpTemp = fopen(this->keyfilename.c_str(), "w");
            rewind(fpTemp);
            fwrite(this->pixels, 1, (size_t)(this->width * this->height * 4), fpTemp);
            memcpy(&(this->shm->buffercount), &(this->buffercount), 8);
            memcpy(&(this->shm->width), &(this->width), 4);
            memcpy(&(this->shm->height), &(this->height), 4);
            fflush(fpTemp);
            fclose(fpTemp);
            memcpy(&(this->shm->writing), &off, 4);
            memcpy(&(this->shm->bufferrequested), &off, 4);
    }
    (this->buffercount)++;

    return 1;
}
