#ifndef _STRING_HH_
#define _STRING_HH_

#include <string>
#include <vector>
#include "fontlib/fontlib.hh"
#include "kolor.hh"
#include "varstring.hh"
#include "geometric.hh"
#include "parent.hh"

class dcString : public dcGeometric
{
    public:
        dcString(dcParent *);
        void setColor(const char *);
        void setBackgroundColor(const char *);
        void setFont(const char *, const char *, const char *, const char *);
        void setShadowOffset(const char *);
        void setString(std::string);
        void draw(void);

    private:
        size_t parse_var(std::string);
        unsigned count_lines(std::string);

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
