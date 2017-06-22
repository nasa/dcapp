#ifndef _CONDITION_HH_
#define _CONDITION_HH_

#include "object.hh"
#include "parent.hh"

class dcCondition : public dcObject
{
    public:
        dcCondition(int, int, void *, int, void *);
        virtual ~dcCondition();

        void handleKeyboard(char);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void updateData(void);
        void processAnimation(void);

        dcParent *TrueList;
        dcParent *FalseList;

    private:
        int opspec;
        int datatype1;
        int datatype2;
        void *val1;
        void *val2;
};

#endif
