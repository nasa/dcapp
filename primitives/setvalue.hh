#ifndef _SETVALUE_HH_
#define _SETVALUE_HH_

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
        void calculateValue(int, int, void *, int, void *, int, void *, int, void *);

        int optype;
        int datatype1;
        int datatype2;
        int mindatatype;
        int maxdatatype;
        void *var;
        void *val;
        void *min;
        void *max;
};

#endif
