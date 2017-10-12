#ifndef _PIXELSTREAMMJPEG_HH_
#define _PIXELSTREAMMJPEG_HH_

#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include "basicutils/timer.hh"
#include "PixelStreamData.hh"

#define CONNECTION_ATTEMPT_INTERVAL 2
#define CONNECTION_TIMEOUT 2.0
#define CONNECTION_TIMEOUT_USEC 10000

class PixelStreamMjpeg : public PixelStreamData
{
    public:
        PixelStreamMjpeg();
        virtual ~PixelStreamMjpeg();

        bool operator == (const PixelStreamMjpeg &);
        bool operator != (const PixelStreamMjpeg &);
        int reader(void);
        int readerInitialize(const char *, int, const char *);

    private:
        int socket_connect(void);
        void socket_disconnect(void);
        int SendDataRequest(const char *);
        int findCrlf(char *, unsigned);
        int findCrlfCrlf(char *, unsigned);
        int RecvHeader(void);
        int RecvData(void);
        void loadPixels(const char *, size_t);

        char *host;
        int port;
        char *path;
        char *data_request;
        struct sockaddr_in server_address;
        int CommSocket;
        Timer *lastconnectattempt;
        Timer *lastread;
        char *readbuf;
        size_t readbufalloc;
        size_t dataalloc;
        int socket_connected;
        int data_requested;
        int header_received;
        int totalbytes;
        int masteroffset;
        bool flushonly;
};

#endif
