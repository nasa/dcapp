#ifndef _PANEL_HH_
#define _PANEL_HH_

#include "kolor.hh"
#include "parent.hh"

class dcPanel : public dcParent
{
    public:
        dcPanel(int, float, float, Kolor *);
        bool checkID(int);
        void draw(void);
    
    private:
        int displayID;
        float orthoX;
        float orthoY;
        Kolor color;
};

#endif
