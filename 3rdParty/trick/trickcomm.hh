#ifndef _TRICKCOMM_HH_
#define _TRICKCOMM_HH_

#include <list>
#include "utils/timer.hh"
#include "comm.hh"
#ifdef TRICKACTIVE
#include "vscomm.hh"
#endif
#include "varlist.hh"

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();
        virtual ~TrickCommModule();

        typedef enum { FromTrick, ToTrick } BufferType;
        typedef enum { AppTerminate, AppReconnect } DisconnectAction;

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void flagAsChanged(void *);
        bool isActive(void);

        void setHost(const char *);
        void setPort(int);
        void setDataRate(const char *);
        void setReconnectOnDisconnect(void);
        int addParameter(int, const char *, const char *, const char *, const char *, int);
        void finishInitialization(void);

    private:
        typedef struct
        {
            int type;
            char *trickvar;
            char *units;
            void *trickvalue;
            void *dcvalue;
            union
            {
                int i;
                float f;
                char str[STRING_DEFAULT_LENGTH];
            } prevvalue;
            bool forcewrite;
            bool init_only;
            int method;
        } io_parameter;

        bool active;
        Timer *last_connect_attempt;
#ifdef TRICKACTIVE
        VariableServerComm *tvs;
#else
        void *tvs;
#endif
        char *host;
        int port;
        char *datarate;
        TrickCommModule::DisconnectAction disconnectaction;
        std::list<io_parameter> fromSim;
        std::list<io_parameter> toSim;

        void activate(void);
};

#endif
