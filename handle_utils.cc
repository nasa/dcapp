#include <cstdlib>
#include <list>
#include "nodes.hh"
#include "types.hh"
#include "primitives/primitives.hh"

extern void UpdateDisplay(void);

extern appdata AppData;

void ProcessEvents(void)
{
    if (!AppData.events.empty())
    {
        std::list<dcObject *>::iterator event;
        for (event = AppData.events.begin(); event != AppData.events.end(); event++)
        {
            (*event)->updateData();
        }
        AppData.events.clear();
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
