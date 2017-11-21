#ifndef _WINDOW_HH_
#define _WINDOW_HH_

#include "parent.hh"
#include "panel.hh"

class dcWindow : public dcParent
{
    public:
        dcWindow();
        void setActiveDisplay(const char *);
        void setCurrentPanel(void);
        void draw(void);
        void handleKeyPress(char);
        void handleKeyRelease(char);
        void handleMousePress(double, double);
        void handleBezelPress(int);
        void updateStreams(unsigned);

// note that handleMouseRelease and handleBezelRelease use the methods from the base
// class since ALL panels are checked in case currentPanel changed after the press event

    private:
        int *displayID;
        dcPanel *currentPanel;
};

#endif
