#ifndef PIXELSTREAM_H
#define PIXELSTREAM_H

// This is a safe size.  The shmget call will fail if it's too small or too large.  It only needs
// to be large enough to house the elements of the PixelStreamData structure defined below.
#define SHM_SIZE 1024

typedef struct
{
    uint32_t writing;
    uint32_t reading;
    uint64_t buffercount;
    uint32_t width;
    uint32_t height;
} PixelStreamData;

#endif
