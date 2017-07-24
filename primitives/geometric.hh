#ifndef _GEOMETRIC_HH_
#define _GEOMETRIC_HH_

#include <string>
#include <vector>
#include "fontlib/fontlib.hh"
#include "kolor.hh"
#include "varstring.hh"
#include "object.hh"
#include "parent.hh"

class dcGeometric : public virtual dcObject
{
    public:
        dcGeometric(void);
        void setPosition(const char *, const char *);
        void setSize(const char *, const char *);
        void setRotation(const char *);
        void setAlignment(const char *, const char *);

    private:
        float *x;
        float *y;
        float *w;
        float *h;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        float *rotate;
};

#endif
