// Generic display logic routines (used when there is no DisplayLogic element set in specfile)

#include "basicutils/msg.hh"

void DisplayPreInitStub(void *(* /* get_pointer */)(const char *))
{
    debug_msg("No DisplayPreInit() element defined - using generic stub instead");
}

void DisplayInitStub(void)
{
    debug_msg("No DisplayInit() element defined - using generic stub instead");
}

void DisplayLogicStub(void)
{
    static int firstpass = 1;

    if (firstpass)
    {
        debug_msg("No DisplayLogic() element defined - using generic stub instead");
        firstpass = 0;
    }
}

void DisplayCloseStub(void)
{
    debug_msg("No DisplayClose() element defined - using generic stub instead");
}
