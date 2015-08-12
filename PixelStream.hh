#ifndef _PIXELSTREAM_HH_
#define _PIXELSTREAM_HH_

#define CONNECTION_TIMEOUT 2.0
#define REQUEST_BUFFER 1

enum { PixelStreamFileProtocol, PixelStreamTcpProtocol };

/*
    Server = EDGE
    Client = dcapp

    ClientToServer (not blocking):
        uint8_t command (REQUEST_BUFFER)
    ServerToClient (blocking):
        uint16_t endianflag
        uint32_t width
        uint32_t height
        pixels
*/

#endif
