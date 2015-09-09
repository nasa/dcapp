#ifndef _PIXELSTREAMTCP_HH_
#define _PIXELSTREAMTCP_HH_

#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include "PixelStreamData.hh"
#include "timer.hh"

#define CONNECTION_ATTEMPT_INTERVAL 2
#define CONNECTION_TIMEOUT_USEC 10000

class PixelStreamTcp : public PixelStreamData
{
    public:
        PixelStreamTcp();
        virtual ~PixelStreamTcp();

        int reader(void);

        int initialize(char *, int);
        char *getHost(void);
        int getPort(void);

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
