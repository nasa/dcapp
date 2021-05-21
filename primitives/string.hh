#ifndef _STRING_HH_
#define _STRING_HH_

#include <string>
#include <vector>
#include "RenderLib/RenderLib.hh"
#include "basicutils/timer.hh"
#include "variables.hh"
#include "values.hh"
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
        std::string get(Value *zeroTrim)
        {
            return var->getString(format, zeroTrim->getDecimal());
        };

    private:
        Variable *var;
        std::string format;
};

class dcString : public dcGeometric
{
    public:
        dcString(dcParent *);
        void setColor(const std::string &);
        void setBackgroundColor(const std::string &);
        void setOutlineColor(const std::string &);
        void setFont(const std::string &, const std::string &, const std::string &, const std::string &);
        void setShadowOffset(const std::string &);
        void setUpdateRate(const std::string &);
        void setZeroTrim(const std::string &);
        void setString(const std::string &);
        void draw(void);

    private:
        size_t parse_var(const std::string &);

        bool background;
        Kolor color;
        Kolor bgcolor;
        Kolor outlineColor;
        std::vector<VarString *> vstring;
        std::vector<std::string> filler;
        tdFont *fontID;
        Value *fontSize;
        Value *shadowOffset;
        flMonoOption forcemono;
        Value *zeroTrim;
        bool trimDefined;
        Value *updateRate;
        bool rateDefined;
        bool outlineDefined;
        Timer lastUpdate;
        std::string storedString;
};

#endif
