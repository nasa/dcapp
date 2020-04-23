#ifndef _PANEL_HH_
#define _PANEL_HH_

#include "valuedata.hh"
#include "kolor.hh"
#include "parent.hh"

class dcPanel : public dcParent
{
    public:
        dcPanel(dcParent *);
        void setID(const char *);
        void setColor(const char *);
        void setOrtho(const char *, const char *);
        bool checkID(int);
        ValueData *getContainerWidth(void);
        ValueData *getContainerHeight(void);
        void draw(void);
        void handleMousePress(double, double);
        void handleMouseMotion(double, double);

    private:
        int displayID;
        ValueData *orthoX;
        ValueData *orthoY;
        Kolor color;
};

#endif
