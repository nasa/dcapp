#ifndef _HAGSTROM_HH_
#define _HAGSTROM_HH_

#include "device.hh"

#ifdef IDF

#include "basicutils/timer.hh"
#include "idf/UsbHagstromKEUSB36FS.hh"

class HagstromDevice : public DeviceModule
{
    public:
        HagstromDevice();
        virtual ~HagstromDevice();

        void setSerialNumber(const char *);
        void read(void);

    private:
        idf::UsbHagstromKEUSB36FS *hagstrom;
        int *currkey;
        int *prevkey;
        Timer last_connect_attempt;
        bool valid;
};

#else

class HagstromDevice : public DeviceModule
{
    public:
        HagstromDevice();

        void setSerialNumber(const char *) { };
};

#endif

#endif
