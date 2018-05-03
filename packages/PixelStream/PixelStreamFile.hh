#ifndef _PIXELSTREAMFILE_HH_
#define _PIXELSTREAMFILE_HH_

#include <cstdio>
#include <stdint.h>
#include "PixelStreamData.hh"

typedef struct
{
    uint32_t writing;
    uint32_t reading;
    uint64_t buffercount;
    uint32_t width;
    uint32_t height;
    uint32_t bufferrequested;
} PixelStreamShmem;

class PixelStreamFile : public PixelStreamData
{
    public:
        PixelStreamFile();
        virtual ~PixelStreamFile();

        bool operator == (const PixelStreamFile &);
        bool operator != (const PixelStreamFile &);
        int reader(void);
        int writer(void);
        bool writeRequested(void);

        int readerInitialize(const char *, int);
        int writerInitialize(const char *, int);
        char *getFileName(void);
        int getShmemKey(void);

    private:
        int genericInitialize(const char *, int);

        char *filename;
        FILE *fp;
        int shmemkey;
        PixelStreamShmem *shm;
        uint64_t buffercount;
};

#endif
