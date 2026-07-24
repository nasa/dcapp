#ifndef DC_APP_PIXELSTREAM_TYPES_H
#define DC_APP_PIXELSTREAM_TYPES_H

typedef enum DcAppPixelstreamType {
    DC_APP_PIXELSTREAM_TYPE_UNDEFINED,
    DC_APP_PIXELSTREAM_TYPE_SHMEM,
    DC_APP_PIXELSTREAM_TYPE_MJPEG
} DcAppPixelstreamType;

typedef int DcAppPixelstreamSourceIndex;

#endif
