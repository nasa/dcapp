#ifndef _UEI_HH_
#define _UEI_HH_

#include "udp_comm.hh"
#include "device.hh"

class UeiDevice : public DeviceModule
{
    public:
        UeiDevice();
        virtual ~UeiDevice();

        void connect(const char *, const char *, const char *);
        void read(void);

    private:
        bool UEI_active;
        bool first;
        int bezelID;
        SocketInfo *uei_socket;
        char *uei_buffer;
};

#endif
