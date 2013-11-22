// Generic display logic routines (used when there is no DisplayLogic element set in specfile)

#include "msg.h"

void DisplayPreInitStub(void *(*get_pointer)(const char *))
{
    debug_msg("No DisplayLogic element defined - using generic stub instead");
}

void DisplayInitStub(void)
{
}

void DisplayLogicStub(void)
{
}
