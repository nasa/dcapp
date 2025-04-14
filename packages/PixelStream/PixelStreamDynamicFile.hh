#ifndef _PIXELSTREAMDYNAMICFILE_HH_
#define _PIXELSTREAMDYNAMICFILE_HH_

#include <string>
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
} PixelStreamDynamicFileShmem;

class PixelStreamDynamicFile : public PixelStreamData
{
    public:
    PixelStreamDynamicFile();
        virtual ~PixelStreamDynamicFile();

        bool operator == (const PixelStreamDynamicFile &);
        bool operator != (const PixelStreamDynamicFile &);
        int reader(void);
        int writer(void);
        bool writeRequested(void);

        int readerInitialize(const std::string &);
        int writerInitialize(const std::string &);

    private:
        int genericInitialize(const std::string &);

        std::string keyfilename;
        PixelStreamDynamicFileShmem *shm;
        uint64_t buffercount;
};

#endif
