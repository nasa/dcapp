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

        typedef enum { FromTrick, ToTrick } BufferType;

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(Variable *);

        void setHost(std::string);
        void setPort(std::string);
        void setDataRate(std::string);
        void setReconnectOnDisconnect(void);
        int addParameter(int, std::string, std::string, std::string, std::string, bool);
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

        void activate(void);
};

#else

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();

        typedef enum { FromTrick, ToTrick } BufferType;

        void setHost(std::string) { };
        void setPort(std::string) { };
        void setDataRate(std::string) { };
        void setReconnectOnDisconnect(void) { };
        int addParameter(int, std::string, std::string, std::string, std::string, bool);
        void finishInitialization(void) { };

    private:
};

#endif

#endif
