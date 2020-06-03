#ifndef _EDGEIO_HH_
#define _EDGEIO_HH_

#include <string>
#include <list>
#include "basicutils/timer.hh"
#include "variables.hh"
#include "comm.hh"
#include "EDGE_rcs.hh"

#define EDGEIO_FROMEDGE           (1)
#define EDGEIO_TOEDGE             (2)

class EdgeCommModule : public CommModule
{
    public:
        EdgeCommModule();
        virtual ~EdgeCommModule();

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(Variable *);

        int addParameter(int, const char *, const char *);
        int finishInitialization(const char *, const char *, double);

    private:
        typedef struct
        {
            std::string edgecmd;
            Variable *currvalue;
            Variable prevvalue;
            bool forcewrite;
        } io_parameter;

        std::list<io_parameter> fromEdge;
        std::list<io_parameter> toEdge;
        Timer *last_connect_attempt;
        Timer *edge_timer;
        double update_rate;
        char *cmd_group;
        EdgeRcsComm *rcs;

        void activate(void);
};

#endif
