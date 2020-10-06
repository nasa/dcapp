#ifndef _SETVALUE_HH_
#define _SETVALUE_HH_

#include "xml_data.hh"
#include "variables.hh"
#include "values.hh"
#include "animation.hh"
#include "object.hh"
#include "parent.hh"

class dcSetValue : public dcObject
{
    public:
        dcSetValue(dcParent *, const xmldata &, const xmldata &);
        void setOperator(const xmldata &);
        void setRange(const xmldata &, const xmldata &);
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
