#ifndef _PIXELSTREAMVSM_HH_
#define _PIXELSTREAMVSM_HH_

#ifdef CURL_ENABLED
#include <curl/curl.h>
#endif
#include "basicutils/timer.hh"
#include "PixelStreamMjpeg.hh"

class PixelStreamVsm : public PixelStreamMjpeg
{
    public:
        PixelStreamVsm();
        virtual ~PixelStreamVsm();

        bool operator == (const PixelStreamVsm &);
        bool operator != (const PixelStreamVsm &);
        int readerInitialize(const char *, int, char *);
        int reader(void);

    private:
        int assignNewCamera(void);
        int resolveURL(const char *);

#ifdef CURL_ENABLED
        CURL *curl;
#else
        void *curl;
#endif
        char *vsmhost;
        int vsmport;
        char *curr_camera;
        char *prev_camera;
        bool cameraassigned;
        bool first_assign_attempt;
        Timer *assigncameraattempt;
};

#endif
