#ifndef _CAN_HH_
#define _CAN_HH_

#include <cstdint>
#ifdef NTCAN
#include "ntcan.h"
#endif
#include "device.hh"

class CanDevice : public DeviceModule
{
    public:
        CanDevice();
        virtual ~CanDevice();

        void initialize(const char *, const char *, const char *, int *);
        void read(void);

    private:
#ifdef NTCAN
        NTCAN_HANDLE ntCanHandle;
#endif
        bool CAN_active;
        int *canbus_inhibited;
        uint32_t buttonID;
        uint32_t controlID;
};

#endif
