#ifndef _WINDOW_HH_
#define _WINDOW_HH_

#include <string>
#include "values.hh"
#include "parent.hh"
#include "panel.hh"

class dcWindow : public dcParent
{
    public:
        dcWindow();
        void setActiveDisplay(const std::string &);
        void setCurrentPanel(void);
        void reshape(double, double);
        void draw(void);
        void handleKeyPress(char);
        void handleKeyRelease(char);
        void handleMousePress(double, double);
        void handleBezelPress(int);
        void handleMouseMotion(double, double);
        void updateStreams(unsigned);

// note that handleMouseRelease and handleBezelRelease use the methods from the base
// class since ALL panels are checked in case currentPanel changed after the press event

    private:
        Value *displayID;
        dcPanel *currentPanel;
};

#endif
