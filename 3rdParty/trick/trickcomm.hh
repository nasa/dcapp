#ifndef _TRICKCOMM_HH_
#define _TRICKCOMM_HH_

#ifdef TRICKACTIVE
#include <string>
#include <list>
#include "basicutils/timer.hh"
#include "vscomm.hh"
#endif
#include "comm.hh"

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();
#ifdef TRICKACTIVE
        virtual ~TrickCommModule();

        typedef enum { FromTrick, ToTrick } BufferType;
        typedef enum { AppTerminate, AppReconnect } DisconnectAction;

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(void *);
        bool isActive(void);
#endif

        void setHost(const char *);
        void setPort(int);
        void setDataRate(const char *);
        void setReconnectOnDisconnect(void);
        int addParameter(int, const char *, const char *, const char *, const char *, int);
        void finishInitialization(void);

    private:
#ifdef TRICKACTIVE
        typedef struct
        {
            int type;
            char *trickvar;
            char *units;
            void *trickvalue;
            void *dcvalue;
            struct
            {
                double decval;
                int intval;
                std::string strval;
            } prevvalue;
            bool forcewrite;
            bool init_only;
            int method;
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
#endif
};

#endif
