#ifndef _PIXELSTREAMVSM_HH_
#define _PIXELSTREAMVSM_HH_

#include "values.hh"
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
        int readerInitialize(const std::string &, int, const std::string &, Value *);
        int reader(void);

    private:
        int assignNewCamera(void);
        int resolveURL(std::string);

        CURL *curl;
        std::string vsmhost;
        int vsmport;
        Value *curr_camera;
        std::string prev_camera;
        std::string userpath;
        bool cameraassigned;
        bool first_assign_attempt;
        Timer *assigncameraattempt;
};

#else

class PixelStreamVsm : public PixelStreamMjpeg
{
    public:
        PixelStreamVsm();
        virtual ~PixelStreamVsm() { };

        bool operator == (const PixelStreamVsm &);
        bool operator != (const PixelStreamVsm &);
        int readerInitialize(const std::string &, int, const std::string &, Value *);
        int reader(void);

    private:
        std::string vsmhost;
        int vsmport;
        Value *curr_camera;
};

#endif

#endif
