#ifndef _PIXELSTREAMVSM_HH_
#define _PIXELSTREAMVSM_HH_

#include "PixelStreamMjpeg.hh"

#if defined(CURL_ENABLED) && defined(JPEG_ENABLED)

#include <string>
#include <curl/curl.h>
#include "basicutils/timer.hh"

class PixelStreamVsm : public PixelStreamMjpeg
{
    public:
        PixelStreamVsm();
        virtual ~PixelStreamVsm();

        bool operator == (const PixelStreamVsm &);
        bool operator != (const PixelStreamVsm &);
        int readerInitialize(const char *, int, const char *, std::string *);
        int reader(void);

    private:
        int assignNewCamera(void);
        int resolveURL(const char *);

        CURL *curl;
        char *vsmhost;
        int vsmport;
        std::string *curr_camera;
        std::string prev_camera;
        char *userpath;
        bool cameraassigned;
        bool first_assign_attempt;
        Timer *assigncameraattempt;
};

#else

#include <string>

class PixelStreamVsm : public PixelStreamMjpeg
{
    public:
        PixelStreamVsm();
        virtual ~PixelStreamVsm();

        bool operator == (const PixelStreamVsm &);
        bool operator != (const PixelStreamVsm &);
        int readerInitialize(const char *, int, const char *, std::string *);
        int reader(void);

    private:
        char *vsmhost;
        int vsmport;
        std::string *curr_camera;
};

#endif

#endif
