#ifndef _CAN_HH_
#define _CAN_HH_

#include "variables.hh"
#include "device.hh"

#ifdef NTCAN

#include <cstdint>
#include "ntcan.h"

class CanDevice : public DeviceModule
{
    public:
        CanDevice();
        virtual ~CanDevice();

        void initialize(const char *, const char *, const char *, const char *);
        void read(void);

    private:
        NTCAN_HANDLE ntCanHandle;
        bool CAN_active;
        Variable *canbus_inhibited;
        uint32_t buttonID;
        uint32_t controlID;
};

#else

class CanDevice : public DeviceModule
{
    public:
        CanDevice();

        void initialize(const char *, const char *, const char *, const char *) { };
};

#endif

#endif
