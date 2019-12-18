#ifndef _EDGE_RCS_HH_
#define _EDGE_RCS_HH_

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class EdgeRcsComm
{
    public:
        EdgeRcsComm();
        virtual ~EdgeRcsComm();

        int initialize(const char *, const char *);
        int send_doug_command(std::string &, char **, char **);

    private:
        int edgercs_active;
        struct addrinfo *server_addr_info;

        int read_rcs_message(int, char **);
};

#endif
