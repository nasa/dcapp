#ifndef _TRICKCOMM_HH_
#define _TRICKCOMM_HH_

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

        void setHost(char *);
        void setPort(int);
        void setDataRate(char *);
        void setReconnectOnDisconnect(void);
        void initializeParameterList(int);
        int addParameter(int, const char *, const char *, const char *, const char *);
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
            int forcewrite;
            int init_only;
        } io_parameter;
        typedef struct
        {
            int count;
            int allocated_elements;
            TrickCommModule::io_parameter *data;
        } io_parameter_list;

        int active;
        char *host;
        int port;
        char *datarate;
        TrickCommModule::DisconnectAction disconnectaction;
        TrickCommModule::io_parameter_list fromsim;
        TrickCommModule::io_parameter_list tosim;
        VariableServerComm *tvs;

        void activate(void);
};

#endif
