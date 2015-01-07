#ifndef _COMM_HH_
#define _COMM_HH_

#include <sys/time.h>

class CommModule
{
    public:
        CommModule();
        virtual ~CommModule();

        typedef enum { None, Inactive, Success, Fail, Terminate } CommStatus;

        virtual CommModule::CommStatus read(void);
        virtual CommModule::CommStatus write(void);
        virtual void flagAsChanged(void *);

        void ResetLastConnectAttemptTime(void);
        float SecondsSinceLastConnectAttempt(void);

    private:
        struct timeval last_connect_attempt;
};

#endif
