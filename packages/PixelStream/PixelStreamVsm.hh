#ifndef _PIXELSTREAMVSM_HH_
#define _PIXELSTREAMVSM_HH_

#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include "basicutils/timer.hh"
#include "PixelStreamData.hh"

#define CAMERA_ASSIGN_INTERVAL 1
#define CONNECTION_ATTEMPT_INTERVAL 2
#define CONNECTION_TIMEOUT 2.0
#define CONNECTION_TIMEOUT_USEC 10000

class PixelStreamVsm : public PixelStreamData
{
    public:
        PixelStreamVsm();
        virtual ~PixelStreamVsm();

        bool operator == (const PixelStreamVsm &);
        bool operator != (const PixelStreamVsm &);
        int reader(void);
        int readerInitialize(const char *, int, char *);

    private:
        int assignNewCamera(void);
        int resolveURL(const char *);
        int socket_connect(void);
        void socket_disconnect(void);
        bool SendDataRequest(const char *);
        int findCrlf(char *, unsigned);
        int findCrlfCrlf(char *, unsigned);
        bool RecvHeader(void);
        bool RecvData(void);
        void loadPixels(const char *, size_t);

        CURL *curl;
        char *curlhost;
        int curlport;
        char *curlcamera;
        char *prevcamera;
        char *data_request;
        struct sockaddr_in server_address;
        int CommSocket;
        bool cameraassigned;
        Timer *assigncameraattempt;
        Timer *lastconnectattempt;
        Timer *lastread;
        char *readbuf;
        size_t readbufalloc;
        size_t dataalloc;
        bool data_requested;
        bool header_received;
        int totalbytes;
        int masteroffset;
        bool flushonly;
};

#endif
