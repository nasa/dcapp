#ifndef _PANEL_HH_
#define _PANEL_HH_

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
        double *getContainerWidth(void);
        double *getContainerHeight(void);
        void draw(void);
        void handleMousePress(double, double);
        void handleMouseMotion(double, double);

    private:
        int displayID;
        double orthoX;
        double orthoY;
        Kolor color;
};

#endif
