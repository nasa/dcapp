#ifndef _PIXELSTREAMMJPEG_HH_
#define _PIXELSTREAMMJPEG_HH_

#include <cstddef>
#include "basicutils/timer.hh"
#include "PixelStreamData.hh"

class PixelStreamMjpeg : public PixelStreamData
{
    public:
        PixelStreamMjpeg();
        virtual ~PixelStreamMjpeg();

        bool operator == (const PixelStreamMjpeg &);
        bool operator != (const PixelStreamMjpeg &);
        int readerInitialize(const char *, int, const char *, const char *, const char *);
        int reader(void);
        void processData(const char *, size_t);
        void updateStatus(void);

    protected:
        void curlConnect(void);
        void curlDisconnect(void);
        void loadPixels(const char *, size_t);

        void *mjpegIO;
        Timer *lastconnectattempt;
        Timer *lastread;
        Timer *lastinview;
        bool connectinprogress;
        bool inview;
        bool updated;

    private:
        char *host;
        int port;
        char *path;
        char *username;
        char *password;
        char *readbuf;
        size_t readbufalloc; // bytes allocated for storing data received from CURL
        size_t pixelsalloc;  // bytes allocated for rendering image
        size_t imagebytes;   // bytes expected for current image
        size_t totalbytes;   // total bytes in readbuf
        size_t masteroffset; // stored pointer within readbuf
        bool readinprogress;
};

#endif
