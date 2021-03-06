#ifndef _COMM_HH_
#define _COMM_HH_

#include "variables.hh"

class CommModule
{
    public:
        CommModule();
        virtual ~CommModule();

        typedef enum { None, Inactive, Success, Fail, Terminate } CommStatus;

        void setConnectedVariable(std::string);
        virtual CommModule::CommStatus read(void);
        virtual CommModule::CommStatus write(void);
        virtual void flagAsChanged(Variable *);
        void updateConnectedVariable(void);

    protected:
        bool active;

    private:
        Variable *activeID;
};

#endif
