#ifndef _PIXELSTREAM_HH_
#define _PIXELSTREAM_HH_

#define DEFAULT_MAX_BUFFER 8192

#define CONNECTION_TIMEOUT 2.0

#define REQUEST_BUFFER 1
#define REQUEST_CHUNK 2
#define SEND_HEARTBEAT 3

#ifndef MIN
#define MIN(a, b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a)<(b)) ? (b) : (a))
#endif

/*
    Server = EDGE
    Client = dcapp

    HANDSHAKE:
        Client:
            send uint16_t endianflag
            send uint32_t maxrecvsize
        Server:
            send uint16_t endianflag
            send uint32_t chunksize
    DATAEXCHANGE:
        Client:
            send uint8_t command (REQUEST_BUFFER)
        Server:
            send uint32_t width;
            send uint32_t height;
        for each chunk:
            Client:
                send uint8_t command (REQUEST_CHUNK) if more data needed
            Server:
                send pixels
*/

#endif
