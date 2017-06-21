#include "panel.hh"

dcPanel::dcPanel(int id)
{
    displayID = id;
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
}
