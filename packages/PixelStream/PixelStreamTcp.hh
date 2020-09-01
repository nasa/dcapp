#ifndef _PIXELSTREAMTCP_HH_
#define _PIXELSTREAMTCP_HH_

/***
    Server = EDGE
    Client = dcapp

    ClientToServer (not blocking):
        uint8_t command (REQUEST_BUFFER)
    ServerToClient (blocking):
        uint16_t endianflag
        uint32_t width
        uint32_t height
        pixels
***/

#include <string>
#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include "basicutils/timer.hh"
#include "PixelStreamData.hh"

class PixelStreamTcp : public PixelStreamData
{
    public:
        PixelStreamTcp();
        virtual ~PixelStreamTcp();

        bool operator == (const PixelStreamTcp &);
        bool operator != (const PixelStreamTcp &);
        int reader(void);
        int writer(void);
        bool writeRequested(void);

        int readerInitialize(const std::string &, int);
        int writerInitialize(int);

    private:
        int read_socket_connect(void);
        int write_socket_connect(int);
        void connect_write_sockets(void);
        void socket_disconnect(int *);
        void disconnect_all(void);
        bool SendDataRequest(void);
        bool RecvHeader(uint32_t *, uint32_t *);
        int RecvData(char *, uint32_t);
        bool RecvDataRequest(void);
        bool SendHeader(uint32_t, uint32_t);
        int SendData(char *, uint32_t);

        std::string host;
        int port;
        struct sockaddr_in server_address;
        int ListenSocket;
        int ClientToServerSocket;
        int ServerToClientSocket;
        Timer *lastconnectattempt;
        Timer *lastread;
        uint32_t datasize;
        size_t dataalloc;
        bool data_requested;
        bool header_sent;
        bool header_received;
        struct pollfd rfd;
        struct pollfd wfd;
};

#endif
