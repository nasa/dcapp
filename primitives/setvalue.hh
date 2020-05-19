#ifndef _SETVALUE_HH_
#define _SETVALUE_HH_

#include "valuedata.hh"
#include "animation.hh"
#include "object.hh"
#include "parent.hh"

class dcSetValue : public dcObject
{
    public:
        dcSetValue(dcParent *, const char *, const char *);
        void setOperator(const char *);
        void setRange(const char *, const char *);
        void draw(void);
        void handleEvent(void);
        void updateData(void);
        void processAnimation(Animation *);

    private:
        void calculateValue(Variable *);

        int optype;
        Variable *var;
        Value *val;
        Value *min;
        Value *max;
};

#endif
