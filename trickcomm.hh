#ifndef _TRICKCOMM_HH_
#define _TRICKCOMM_HH_

#include "comm.hh"

class TrickCommModule : public CommModule
{
    public:
        TrickCommModule();
        virtual ~TrickCommModule();

        typedef enum { AppTerminate, AppReconnect } DisconnectAction;

        char *host;
        int port;
        char *datarate;
        TrickCommModule::DisconnectAction disconnectaction;

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);
        void setForceWrite(void *);

        void setHost(char *, unsigned);
        void setPort(int, unsigned);
        void setDataRate(char *);
        void setReconnectOnDisconnect(void);
        void initializeParameterList(int);
        int addParameter(int, const char *, const char *, const char *, const char *);
        void finishInitialization(void);
        CommModule::CommStatus activate(void);
};

#endif
