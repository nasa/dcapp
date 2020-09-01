#ifndef _UEI_HH_
#define _UEI_HH_

#include <string>
#include "udp_comm.hh"
#include "device.hh"

class UeiDevice : public DeviceModule
{
    public:
        UeiDevice();
        virtual ~UeiDevice();

        void connect(const std::string &, const std::string &);
        void read(void);

    private:
        bool UEI_active;
        bool first;
        int bezelID;
        SocketInfo *uei_socket;
        char *uei_buffer;
};

#endif
