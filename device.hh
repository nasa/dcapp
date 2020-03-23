#ifndef _DEVICE_HH_
#define _DEVICE_HH_

class DeviceModule
{
    public:
        DeviceModule();
        virtual ~DeviceModule();

        virtual void read(void);
};

#endif
