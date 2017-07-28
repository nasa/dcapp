#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <strings.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "PixelStream/PixelStream.hh"
#include "dc.hh"
#include "varlist.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "opengl_draw.hh"
#include "alignment.hh"

extern appdata AppData;

bool check_dynamic_element(const char *spec)
{
    if (spec)
    {
        if (strlen(spec) > 1)
        {
            if (spec[0] == '@') return true;
        }
    }
    return false;
}

float *getFloatPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (float *)get_pointer(valstr);
    else return dcLoadConstant(StrToFloat(valstr, 0));
}

int *getIntegerPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (int *)get_pointer(valstr);
    else return dcLoadConstant(StrToInt(valstr, 0));
}

char *getStringPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (char *)get_pointer(valstr);
    else return dcLoadConstant(valstr);
}

void *getVariablePointer(int datatype, const char *valstr)
{
    switch (datatype)
    {
        case FLOAT_TYPE:
            return getFloatPointer(valstr);
            break;
        case INTEGER_TYPE:
            return getIntegerPointer(valstr);
            break;
        case STRING_TYPE:
            return getStringPointer(valstr);
            break;
    }
    return 0x0;
}

int get_data_type(const char *valstr)
{
    if (check_dynamic_element(valstr)) return get_datatype(valstr);
    return UNDEFINED_TYPE;
}

void new_button(dcContainer *myitem, const char *type, const char *switchid, const char *switchonval, const char *switchoffval, const char *indid, const char *indonval,
                const char *activevar, const char *activeval, const char *transitionid, const char *key, const char *keyascii, const char *bezelkey)
{
    bool toggle = false, momentary = false;
    const char *offval, *zerostr=strdup("0"), *onestr=strdup("1");

    if (type)
    {
        if (!strcmp(type, "Toggle")) toggle = true;
        if (!strcmp(type, "Momentary")) momentary = true;
    }

    dcParent *mySublist = myitem;
    if (activevar)
    {
        if (!activeval) activeval = onestr;
        dcCondition *mycond = new dcCondition(myitem, "eq", activevar, activeval);
        mySublist = mycond->TrueList;
    }

    if (!switchonval) switchonval = onestr;
    if (toggle || momentary)
    {
        if (switchoffval) offval = switchoffval;
        else offval = zerostr;
    }
    else offval = 0x0;

    if (toggle)
    {
        dcCondition *mycond = new dcCondition(mySublist, "eq", indid, indonval);
        dcMouseEvent *mymouse = new dcMouseEvent(mycond->TrueList);
        new dcSetValue(mymouse->PressList, switchid, offval);
        if (transitionid) new dcSetValue(mymouse->PressList, transitionid, "-1");
        dcMouseEvent *mymouse1 = new dcMouseEvent(mycond->FalseList);
        new dcSetValue(mymouse1->PressList, switchid, switchonval);
        if (transitionid) new dcSetValue(mymouse1->PressList, transitionid, "1");
        if (key || keyascii)
        {
            dcKeyboardEvent *myevent = new dcKeyboardEvent(mycond->TrueList, key, keyascii);
            new dcSetValue(myevent->PressList, switchid, offval);
            if (transitionid) new dcSetValue(myevent->PressList, transitionid, "-1");
            myevent = new dcKeyboardEvent(mycond->FalseList, key, keyascii);
            new dcSetValue(myevent->PressList, switchid, switchonval);
            if (transitionid) new dcSetValue(myevent->PressList, transitionid, "1");
        }
        if (bezelkey)
        {
            dcBezelEvent *myevent1 = new dcBezelEvent(mycond->TrueList, bezelkey);
            new dcSetValue(myevent1->PressList, switchid, offval);
            if (transitionid) new dcSetValue(myevent1->PressList, transitionid, "-1");
            dcBezelEvent *myevent2 = new dcBezelEvent(mycond->FalseList, bezelkey);
            new dcSetValue(myevent2->PressList, switchid, switchonval);
            if (transitionid) new dcSetValue(myevent2->PressList, transitionid, "1");
        }
    }
    else
    {
        dcMouseEvent *mymouse = new dcMouseEvent(mySublist);
        new dcSetValue(mymouse->PressList, switchid, switchonval);
        if (transitionid) new dcSetValue(mymouse->PressList, transitionid, "1");
        if (momentary)
        {
            new dcSetValue(mymouse->ReleaseList, switchid, offval);
            if (transitionid) new dcSetValue(mymouse->ReleaseList, transitionid, "-1");
        }
        if (key || keyascii)
        {
            dcKeyboardEvent *myevent = new dcKeyboardEvent(mySublist, key, keyascii);
            new dcSetValue(myevent->PressList, switchid, switchonval);
            if (transitionid) new dcSetValue(myevent->PressList, transitionid, "1");
            if (momentary)
            {
                new dcSetValue(myevent->ReleaseList, switchid, offval);
                if (transitionid) new dcSetValue(myevent->ReleaseList, transitionid, "-1");
            }
        }
        if (bezelkey)
        {
            dcBezelEvent *myevent1 = new dcBezelEvent(mySublist, bezelkey);
            new dcSetValue(myevent1->PressList, switchid, switchonval);
            if (transitionid) new dcSetValue(myevent1->PressList, transitionid, "1");
            if (momentary)
            {
                new dcSetValue(myevent1->ReleaseList, switchid, offval);
                if (transitionid) new dcSetValue(myevent1->ReleaseList, transitionid, "-1");
            }
        }
    }

    if (transitionid)
    {
        dcCondition *mylist1, *mylist2, *mylist3;

        mylist1 = new dcCondition(myitem, "eq", transitionid, "1");
        mylist2 = new dcCondition(mylist1->TrueList, "eq", indid, indonval);
        new dcSetValue(mylist2->TrueList, transitionid, "0");
        mylist3 = new dcCondition(mylist2->FalseList, "eq", switchid, switchonval);
        new dcSetValue(mylist3->FalseList, transitionid, "0");

        mylist1 = new dcCondition(myitem, "eq", transitionid, "-1");
        mylist2 = new dcCondition(mylist1->TrueList, "eq", indid, indonval);
        new dcSetValue(mylist2->FalseList, transitionid, "0");
        mylist3 = new dcCondition(mylist2->FalseList, "eq", switchid, switchoffval);
        new dcSetValue(mylist3->TrueList, transitionid, "0");
    }
}
