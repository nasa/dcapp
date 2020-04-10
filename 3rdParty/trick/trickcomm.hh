#ifndef _TRICKCOMM_HH_
#define _TRICKCOMM_HH_

#include "comm.hh"

#ifdef TRICKACTIVE

#include <list>
#include "basicutils/timer.hh"
#include "valuedata.hh"
#include "vscomm.hh"

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();
        virtual ~TrickCommModule();

        typedef enum { FromTrick, ToTrick } BufferType;

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(void *);
        bool isActive(void);

        void setHost(const char *);
        void setPort(int);
        void setDataRate(const char *);
        void setReconnectOnDisconnect(void);
        int addParameter(int, const char *, const char *, const char *, const char *, bool);
        void finishInitialization(void);

    private:
        typedef enum { AppTerminate, AppReconnect } DisconnectAction;

        typedef struct
        {
            char *trickvar;
            char *units;
            ValueData *trickvalue;
            ValueData *currvalue;
            ValueData prevvalue;
            bool forcewrite;
            bool init_only;
            bool method;
        } io_parameter;

        bool active;
        Timer *last_connect_attempt;
        VariableServerComm *tvs;
        char *host;
        int port;
        char *datarate;
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

        void setHost(const char *) { };
        void setPort(int) { };
        void setDataRate(const char *) { };
        void setReconnectOnDisconnect(void) { };
        int addParameter(int, const char *, const char *, const char *, const char *, int);
        void finishInitialization(void) { };

    private:
};

#endif

#endif
