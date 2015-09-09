#ifndef _PIXELSTREAMFILE_HH_
#define _PIXELSTREAMFILE_HH_

#include <stdio.h>
#include <stdint.h>
#include "PixelStreamData.hh"

// This is a safe size.  The shmget call will fail if it's too small or too large.  It only needs
// to be large enough to house the elements of the PixelStreamShmem structure defined below.
#define SHM_SIZE 1024

typedef struct
{
    uint32_t writing;
    uint32_t reading;
    uint64_t buffercount;
    uint32_t width;
    uint32_t height;
} PixelStreamShmem;

class PixelStreamFile : public PixelStreamData
{
    public:
        PixelStreamFile();
        virtual ~PixelStreamFile();

        int reader(void);

        int initialize(char *, int);
        char *getFileName(void);
        int getShmemKey(void);

    private:
        char *filename;
        FILE *fp;
        int shmemkey;
        PixelStreamShmem *shm;
        uint32_t buffercount;
};

#endif
