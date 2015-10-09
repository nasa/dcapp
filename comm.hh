#ifndef _COMM_HH_
#define _COMM_HH_

class CommModule
{
    public:
        CommModule();
        virtual ~CommModule();

        typedef enum { None, Inactive, Success, Fail, Terminate } CommStatus;

        virtual CommModule::CommStatus read(void);
        virtual CommModule::CommStatus write(void);
        virtual void flagAsChanged(void *);
        virtual bool isActive(void);

        int *activeID;
};

#endif
