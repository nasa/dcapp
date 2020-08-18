#ifndef _PANEL_HH_
#define _PANEL_HH_

#include <string>
#include "values.hh"
#include "kolor.hh"
#include "parent.hh"

class dcPanel : public dcParent
{
    public:
        dcPanel(dcParent *);
        void setID(const std::string &);
        void setColor(const std::string &);
        void setOrtho(const std::string &, const std::string &);
        bool checkID(int);
        Value *getContainerWidth(void);
        Value *getContainerHeight(void);
        void draw(void);
        void handleMousePress(double, double);
        void handleMouseMotion(double, double);

    private:
        int displayID;
        Value *orthoX;
        Value *orthoY;
        Kolor color;
};

#endif
