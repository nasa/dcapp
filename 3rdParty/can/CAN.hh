#ifndef _CAN_HH_
#define _CAN_HH_

#include <string>
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

        void initialize(const std::string &, const std::string &, const std::string &, const std::string &);
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

        void initialize(const std::string &, const std::string &, const std::string &, const std::string &) { };
};

#endif

#endif
