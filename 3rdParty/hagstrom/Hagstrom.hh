#ifndef _HAGSTROM_HH_
#define _HAGSTROM_HH_

#include "basicutils/timer.hh"
#ifdef IDF
#include "idf/UsbHagstromKEUSB36FS.hh"
#endif
#include "device.hh"

class HagstromDevice : public DeviceModule
{
    public:
        HagstromDevice();
        virtual ~HagstromDevice();

        void setSerialNumber(const char *);
        void read(void);

    private:
#ifdef IDF
        idf::UsbHagstromKEUSB36FS *hagstrom;
#endif
        int *currkey;
        int *prevkey;
        Timer last_connect_attempt;
        bool valid;
};

#endif
