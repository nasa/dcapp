#ifndef _STRING_HH_
#define _STRING_HH_

#include <string>
#include <vector>
#include "fontlib/fontlib.hh"
#include "kolor.hh"
#include "varstring.hh"
#include "object.hh"

class dcString : public dcObject
{
    public:
        dcString(float *, float *, float *, float *, float *, float *, unsigned, unsigned, float *, bool, Kolor *, Kolor *, std::vector<VarString *>, std::vector<std::string>, flFont *, float *, float *, flMonoOption);

        void draw(void);

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
        bool background;
        Kolor color;
        Kolor bgcolor;
        std::vector<VarString *> vstring;
        std::vector<std::string> filler;
        flFont *fontID;
        float *fontSize;
        float *shadowOffset;
        flMonoOption forcemono;
};

#endif
