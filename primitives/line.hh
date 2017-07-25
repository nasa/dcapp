#ifndef _LINE_HH_
#define _LINE_HH_

#include "kolor.hh"
#include "parent.hh"

class dcLine : public dcParent
{
    public:
        dcLine(dcParent *);
        void setColor(const char *);
        void setLineWidth(const char *);
        void draw(void);
    
    private:
        float linewidth;
        Kolor color;
};

#endif
