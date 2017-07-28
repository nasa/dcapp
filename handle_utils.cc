#include <cstdlib>
#include <cstring>
#include <list>
#include "primitives/primitives.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "animation.hh"

extern void UpdateDisplay(void);

extern appdata AppData;

std::list<dcObject *> events;

void ProcessEvents(void)
{
    if (!events.empty())
    {
        std::list<dcObject *>::iterator event;
        for (event = events.begin(); event != events.end(); event++)
        {
            (*event)->updateData();
        }
        events.clear();
        UpdateDisplay();
    }
}

float getFloatVal(int type, const void *val)
{
    switch (type)
    {
        case FLOAT_TYPE:
            return *(float *)val;
        case INTEGER_TYPE:
            return (float)(*(int *)val);
        case STRING_TYPE:
            return strtof((char *)val, 0x0);
    }
    return 0;
}

int getIntegerVal(int type, const void *val)
{
    switch (type)
    {
        case FLOAT_TYPE:
            return (int)(*(float *)val);
        case INTEGER_TYPE:
            return *(int *)val;
        case STRING_TYPE:
            return strtol((char *)val, 0x0, 10);
    }
    return 0;
}
