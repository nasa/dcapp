#ifndef _EDGE_RCS_HH_
#define _EDGE_RCS_HH_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class EdgeRcsComm
{
    public:
        EdgeRcsComm();
        virtual ~EdgeRcsComm();

        int initialize(char *, char *);
        int send_doug_command(const char *, char **, char **);

    private:
        int edgercs_active;
        struct addrinfo *server_addr_info;

        int read_rcs_message(int, char **);
};

#endif
