#ifndef _LINE_HH_
#define _LINE_HH_

#include <string>
#include "kolor.hh"
#include "parent.hh"

class dcLine : public dcParent
{
    public:
        dcLine(dcParent *);
        void setColor(const std::string &);
        void setLineWidth(const std::string &);
        void setPattern(const std::string &);
        void setFactor(const std::string &);
        void draw(void);
    
    private:
        double linewidth;
        Kolor color;
        uint16_t pattern;
        int factor;
};

#endif
