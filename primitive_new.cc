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

dcSetValue *new_setvalue(const char *, const char *, const char *, const char *, const char *);

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
dcSetValue *myset;

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
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
mymouse->PressList->addChild(myset);
        if (transitionid)
        {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
mymouse->PressList->addChild(myset);
        }
dcMouseEvent *mymouse1 = new dcMouseEvent(mycond->FalseList);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
mymouse1->PressList->addChild(myset);
        if (transitionid)
        {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
mymouse1->PressList->addChild(myset);
        }
        if (key || keyascii)
        {
dcKeyboardEvent *myevent = new dcKeyboardEvent(mycond->TrueList, key, keyascii);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
myevent->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
myevent->PressList->addChild(myset);
            }
myevent = new dcKeyboardEvent(mycond->FalseList, key, keyascii);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
myevent->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
myevent->PressList->addChild(myset);
            }
        }
        if (bezelkey)
        {
dcBezelEvent *myevent1 = new dcBezelEvent(mycond->TrueList, bezelkey);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
myevent1->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
myevent1->PressList->addChild(myset);
            }
dcBezelEvent *myevent2 = new dcBezelEvent(mycond->FalseList, bezelkey);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
myevent2->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
myevent2->PressList->addChild(myset);
            }
        }
    }
    else
    {
dcMouseEvent *mymouse = new dcMouseEvent(mySublist);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
mymouse->PressList->addChild(myset);
        if (transitionid)
        {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
mymouse->PressList->addChild(myset);
        }
        if (momentary)
        {
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
mymouse->ReleaseList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
mymouse->ReleaseList->addChild(myset);
            }
        }
        if (key || keyascii)
        {
dcKeyboardEvent *myevent = new dcKeyboardEvent(mySublist, key, keyascii);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
myevent->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
myevent->PressList->addChild(myset);
            }
            if (momentary)
            {
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
myevent->ReleaseList->addChild(myset);
                if (transitionid)
                {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
myevent->ReleaseList->addChild(myset);
                }
            }
        }
        if (bezelkey)
        {
dcBezelEvent *myevent1 = new dcBezelEvent(mySublist, bezelkey);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
myevent1->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
myevent1->PressList->addChild(myset);
            }
            if (momentary)
            {
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
myevent1->ReleaseList->addChild(myset);
                if (transitionid)
                {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
myevent1->ReleaseList->addChild(myset);
                }
            }
        }
    }

    if (transitionid)
    {
dcCondition *mylist1, *mylist2, *mylist3, *mylist4, *mylist5, *mylist6;
        mylist1 = new dcCondition(myitem, "eq", transitionid, "1");
        mylist2 = new dcCondition(mylist1->TrueList, "eq", indid, indonval);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist2->TrueList->addChild(myset);
        mylist3 = new dcCondition(mylist2->FalseList, "eq", switchid, switchonval);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist3->FalseList->addChild(myset);

        mylist4 = new dcCondition(myitem, "eq", transitionid, "-1");
        mylist5 = new dcCondition(mylist4->TrueList, "eq", indid, indonval);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist5->FalseList->addChild(myset);
        mylist6 = new dcCondition(mylist5->FalseList, "eq", switchid, switchoffval);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist6->TrueList->addChild(myset);
    }
}

struct ModifyValue get_setvalue_data(const char *varspec, const char *opspec, const char *minspec, const char *maxspec, const char *valspec)
{
    struct ModifyValue ret;

    ret.optype = Equals;
    ret.datatype1 = get_data_type(varspec);
    ret.datatype2 = get_data_type(valspec);
    ret.mindatatype = 0;
    ret.maxdatatype = 0;
    ret.var = 0x0;
    ret.val = 0x0;
    ret.min = 0x0;
    ret.max = 0x0;

    if (ret.datatype1 != UNDEFINED_TYPE)
    {
        if (!opspec) ret.optype = Equals;
        else if (!strcmp(opspec, "+=")) ret.optype = PlusEquals;
        else if (!strcmp(opspec, "-=")) ret.optype = MinusEquals;

        ret.var = getVariablePointer(ret.datatype1, varspec);

        if (ret.datatype2 == UNDEFINED_TYPE) ret.datatype2 = ret.datatype1;
        ret.val = getVariablePointer(ret.datatype2, valspec);

        if (minspec)
        {
            ret.mindatatype = get_data_type(minspec);
            if (ret.mindatatype == UNDEFINED_TYPE) ret.mindatatype = ret.datatype1;
            ret.min = getVariablePointer(ret.mindatatype, minspec);
        }

        if (maxspec)
        {
            ret.maxdatatype = get_data_type(maxspec);
            if (ret.maxdatatype == UNDEFINED_TYPE) ret.maxdatatype = ret.datatype1;
            ret.max = getVariablePointer(ret.maxdatatype, maxspec);
        }
    }
    return ret;
}

dcSetValue *new_setvalue(const char *var, const char *optype, const char *min, const char *max, const char *val)
{
    struct ModifyValue myset = get_setvalue_data(var, optype, min, max, val);

    if (myset.datatype1 == UNDEFINED_TYPE) return 0x0;

    return new dcSetValue(myset.optype, myset.datatype1, myset.datatype2, myset.mindatatype, myset.maxdatatype, myset.var, myset.val, myset.min, myset.max);
}
