#ifndef _PIXELSTREAM_HH_
#define _PIXELSTREAM_HH_

enum { PixelStreamFileProtocol, PixelStreamTcpProtocol };

///////////////////////// TCP PROTOCOL /////////////////////////
// Move this to PixelStreamTcp.hh

#define CONNECTION_TIMEOUT 2.0
#define REQUEST_BUFFER 1

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
