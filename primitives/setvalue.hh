#ifndef _SETVALUE_HH_
#define _SETVALUE_HH_

#include "animation.hh"
#include "object.hh"
#include "parent.hh"

class dcSetValue : public dcObject
{
    public:
        dcSetValue(int, int, int, int, int, void *, void *, void *, void *);

        void draw(void);
        void handleEvent(void);
        void updateData(void);
        void processAnimation(Animation *);

    private:
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
