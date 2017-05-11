#ifndef _PIXELSTREAMMJPEG_HH_
#define _PIXELSTREAMMJPEG_HH_

#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include "utils/timer.hh"
#include "PixelStreamData.hh"

#define CONNECTION_ATTEMPT_INTERVAL 2
#define CONNECTION_TIMEOUT 2.0
#define CONNECTION_TIMEOUT_USEC 10000
#define MAX_ATTEMPTS 1000
#define READBUF_ALLOC_CHUNK 256

enum { crlf, crlfcrlf };

class PixelStreamMjpeg : public PixelStreamData
{
    public:
        PixelStreamMjpeg();
        virtual ~PixelStreamMjpeg();

        bool operator == (const PixelStreamMjpeg &);
        bool operator != (const PixelStreamMjpeg &);
        int reader(void);
        int readerInitialize(char *, int);
        char *getHost(void);
        int getPort(void);

    private:
        int socket_connect(void);
        void socket_disconnect(void);
        int SendDataRequest(const char *);
        int GetBuffer(unsigned);
        int RecvHeader(void);
        int RecvData(void);

        char *host;
        int port;
        struct sockaddr_in server_address;
        int CommSocket;
        Timer *lastconnectattempt;
        Timer *lastread;
        char *readbuf;
        void *rawdata;
        size_t readbufalloc;
        size_t rawalloc;
        size_t dataalloc;
        int socket_connected;
        int data_requested;
        int header_received;
};

#endif
