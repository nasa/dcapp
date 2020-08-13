#ifndef _EDGEIO_HH_
#define _EDGEIO_HH_

#include <string>
#include <list>
#include "basicutils/timer.hh"
#include "variables.hh"
#include "comm.hh"
#include "EDGE_rcs.hh"

class EdgeCommModule : public CommModule
{
    public:
        EdgeCommModule();
        virtual ~EdgeCommModule();

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(Variable *);

        void activateFromList(void) { io_map = &(this->fromEdge); };
        void activateToList(void) { io_map = &(this->toEdge); };
        void deactivateList(void) { io_map = 0x0; };
        int addParameter(std::string, std::string);
        int finishInitialization(std::string, std::string, double);

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
        std::list<io_parameter> *io_map;
        Timer *last_connect_attempt;
        Timer *edge_timer;
        double update_rate;
        char *cmd_group;
        EdgeRcsComm *rcs;

        void activate(void);
};

#endif
