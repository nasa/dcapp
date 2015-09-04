#ifndef _PIXELSTREAM_HH_
#define _PIXELSTREAM_HH_

enum { PixelStreamFileProtocol, PixelStreamTcpProtocol };

///////////////////////// FILE PROTOCOL /////////////////////////

// This is a safe size.  The shmget call will fail if it's too small or too large.  It only needs
// to be large enough to house the elements of the PixelStreamData structure defined below.
#define SHM_SIZE 1024

///////////////////////// TCP PROTOCOL /////////////////////////

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
