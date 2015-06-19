#ifndef _EDGEIO_HH_
#define _EDGEIO_HH_

#include <sys/time.h>
#include "comm.hh"
#include "EDGE_rcs.hh"
#include "varlist.hh"
#include "timer.hh"

#define EDGEIO_FROMEDGE           (1)
#define EDGEIO_TOEDGE             (2)

class EdgeCommModule : public CommModule
{
    public:
        EdgeCommModule();
        virtual ~EdgeCommModule();

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(void *);
        int isActive(void);

        int addParameter(int, const char *, const char *);
        int finishInitialization(char *, char *, float);

    private:
        typedef struct
        {
            int type;
            char *edgecmd;
            void *dcvalue;
            union
            {
                int i;
                float f;
                char str[STRING_DEFAULT_LENGTH];
            } prevvalue;
            int forcewrite;
        } io_parameter;

        typedef struct
        {
            int count;
            int allocated_elements;
            io_parameter *data;
        } io_parameter_list;

        int active;
        io_parameter_list fromedge;
        io_parameter_list toedge;
        Timer last_connect_attempt;
        Timer edge_timer;
        float update_rate;
        char *cmd_group;
        EdgeRcsComm *rcs;

        void activate(void);
};

#endif
