#ifndef _STRING_HH_
#define _STRING_HH_

#include <string>
#include <vector>
#include "RenderLib/RenderLib.hh"
#include "valuedata.hh"
#include "kolor.hh"
#include "geometric.hh"
#include "parent.hh"

class VarString
{
    public:
        VarString(Variable *varin, std::string formatin)
        {
            var = varin;
            format = formatin;
        };
        std::string get(void)
        {
            return var->getString(format);
        };

    private:
        Variable *var;
        std::string format;
};

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

        bool background;
        Kolor color;
        Kolor bgcolor;
        std::vector<VarString *> vstring;
        std::vector<std::string> filler;
        tdFont *fontID;
        Value *fontSize;
        Value *shadowOffset;
        flMonoOption forcemono;
};

#endif
