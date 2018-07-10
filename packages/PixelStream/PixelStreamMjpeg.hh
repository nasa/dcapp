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
        size_t imagebytes;
        char *readbuf;
        size_t readbufalloc;
        size_t pixelsalloc;
        int totalbytes;
        int masteroffset;
        bool readinprogress;
};

#endif
