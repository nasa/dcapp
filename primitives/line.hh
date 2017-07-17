#ifndef _LINE_HH_
#define _LINE_HH_

#include "kolor.hh"
#include "opengl_draw.hh"
#include "parent.hh"

class dcLine : public dcParent
{
    public:
        dcLine(float, Kolor *);
        void draw(void);
    
    private:
        float linewidth;
        Kolor color;
};

#endif
