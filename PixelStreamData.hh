#ifndef _PIXELSTREAMDATA_HH_
#define _PIXELSTREAMDATA_HH_

#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include "timer.hh"

#define CONNECTION_ATTEMPT_INTERVAL 2
#define CONNECTION_TIMEOUT_USEC 10000

class PixelStreamData
{
    public:
        PixelStreamData();
        virtual ~PixelStreamData();

        int initialize(char *, int);
        int update(void);
        char *getHost(void);
        int getPort(void);
        int isConnected(void);

        void *pixels;
        uint32_t width;
        uint32_t height;

    private:
        int socket_connect(void);
        void socket_disconnect(int *);
        void disconnect_all(void);
        int SendDataRequest(void);
        int RecvHeader(uint32_t *, uint32_t *);
        int RecvData(char *, uint32_t);

        char *host;
        int port;
        struct sockaddr_in server_address;
        int ClientToServerSocket;
        int ServerToClientSocket;
        Timer lastconnectattempt;
        Timer lastread;
        uint32_t datasize;
        size_t dataalloc;
        int sockets_connected;
        int data_requested;
        int header_received;
        struct pollfd rfd;
        struct pollfd wfd;
};

#endif
