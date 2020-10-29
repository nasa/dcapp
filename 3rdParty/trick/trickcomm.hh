#ifndef _TRICKCOMM_HH_
#define _TRICKCOMM_HH_

#include <string>
#include "comm.hh"

#ifdef TRICKACTIVE

#include <list>
#include "basicutils/timer.hh"
#include "variables.hh"
#include "vscomm.hh"

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();
        virtual ~TrickCommModule();

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(Variable *);

        void setHost(const std::string &);
        void setPort(const std::string &);
        void setDataRate(const std::string &);
        void setReconnectOnDisconnect(void);
        void activateFromList(void) { io_map = &(this->fromSim); };
        void activateToList(void) { io_map = &(this->toSim); };
        void deactivateList(void) { io_map = 0x0; };
        int addParameter(const std::string &, const std::string &, const std::string &, const std::string &, bool);
        void finishInitialization(void);

    private:
        typedef enum { AppTerminate, AppReconnect } DisconnectAction;

        typedef struct
        {
            std::string trickvar;
            std::string units;
            Variable *trickvalue;
            Variable *currvalue;
            Variable prevvalue;
            bool forcewrite;
            bool init_only;
            bool method;
        } io_parameter;

        Timer *last_connect_attempt;
        VariableServerComm *tvs;
        std::string host;
        int port;
        std::string datarate;
        TrickCommModule::DisconnectAction disconnectaction;
        std::list<io_parameter> fromSim;
        std::list<io_parameter> toSim;
        std::list<io_parameter> *io_map;

        void activate(void);
};

#else

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();

        void setHost(const std::string &) { };
        void setPort(const std::string &) { };
        void setDataRate(const std::string &) { };
        void setReconnectOnDisconnect(void) { };
        void activateFromList(void) { };
        void activateToList(void) { };
        void deactivateList(void) { };
        int addParameter(const std::string &, const std::string &, const std::string &, const std::string &, bool) { return this->Inactive; };
        void finishInitialization(void) { };

    private:
};

#endif

#endif
