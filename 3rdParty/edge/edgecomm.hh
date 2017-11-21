#ifndef _EDGEIO_HH_
#define _EDGEIO_HH_

#include <list>
#include "basicutils/timer.hh"
#include "comm.hh"
#include "varlist.hh"
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
        void flagAsChanged(void *);
        bool isActive(void);

        int addParameter(int, const char *, const char *);
        int finishInitialization(const char *, const char *, double);

    private:
        typedef struct
        {
            int type;
            char *edgecmd;
            void *dcvalue;
            union
            {
                int i;
                double f;
                char str[STRING_DEFAULT_LENGTH];
            } prevvalue;
            bool forcewrite;
        } io_parameter;

        bool active;
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
