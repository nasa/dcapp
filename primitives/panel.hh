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
        float *getContainerWidth(void);
        float *getContainerHeight(void);
        void draw(void);
        void handleMousePress(float, float);

    private:
        int displayID;
        float orthoX;
        float orthoY;
        Kolor color;
};

#endif
