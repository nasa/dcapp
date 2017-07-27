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

struct node *new_mouseevent(dcMouseEvent **, dcParent *, struct node **, const char *, const char *, const char *, const char *, const char *, const char *);
struct node *new_keyboardevent(dcKeyboardEvent **, struct node **, const char *, const char *);
struct node *new_bezelevent(dcBezelEvent **, struct node **, const char *);
dcSetValue *new_setvalue(const char *, const char *, const char *, const char *, const char *);
struct node *new_isequal(dcCondition **, struct node **, const char *, const char *, const char *);

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

static void *getVariablePointer(int datatype, const char *valstr)
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

static struct node *add_primitive_node(struct node **list)
{
    struct node *data = (struct node *)calloc(1, sizeof(struct node));
    *list = data;
    return data;
}

struct node *new_container(struct node **list)
{
    struct node *data = add_primitive_node(list);
    return data;
}

struct node *new_button(dcContainer *myitem, struct node **list, const char *x, const char *y, const char *width, const char *height, const char *halign, const char *valign,
                        const char *rotate, const char *type, const char *switchid, const char *switchonval, const char *switchoffval, const char *indid, const char *indonval,
                        const char *activevar, const char *activeval, const char *transitionid, const char *key, const char *keyascii, const char *bezelkey)
{
    int toggle=0, momentary=0;
    struct node *cond, *event, *curlist, **sublist, *list1, *list2, *list3, *list4, *list5, *list6;
    const char *offval, *zerostr=strdup("0"), *onestr=strdup("1");
    struct node *data = add_primitive_node(list);
dcSetValue *myset;

    if (type)
    {
        if (!strcmp(type, "Toggle")) toggle = 1;
        if (!strcmp(type, "Momentary")) momentary = 1;
    }

    data->object.cont.vwidth = data->info.w;
    data->object.cont.vheight = data->info.h;

    curlist = data;
    sublist = &(data->object.cont.SubList);
dcParent *mySublist = myitem;
    if (activevar)
    {
        if (!activeval) activeval = onestr;
dcCondition *mycond;
curlist = new_isequal(&mycond, &(data->object.cont.SubList), "eq", activevar, activeval);
myitem->addChild(mycond);
mySublist = mycond->TrueList;
        sublist = &(curlist->object.cond.TrueList);
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
dcCondition *mycond;
        cond = new_isequal(&mycond, sublist, "eq", indid, indonval);
mySublist->addChild(mycond);
dcMouseEvent *mymouse;
        event = new_mouseevent(&mymouse, mycond->TrueList, &(cond->object.cond.TrueList), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
mymouse->PressList->addChild(myset);
        if (transitionid)
        {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
mymouse->PressList->addChild(myset);
        }
dcMouseEvent *mymouse1;
        event = new_mouseevent(&mymouse1, mycond->FalseList, &(cond->object.cond.FalseList), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, switchonval);
mymouse1->PressList->addChild(myset);
        if (transitionid)
        {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "1");
mymouse1->PressList->addChild(myset);
        }
        if (key || keyascii)
        {
dcKeyboardEvent *myevent;
            event = new_keyboardevent(&myevent, &(cond->object.cond.TrueList), key, keyascii);
mycond->TrueList->addChild(myevent);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
myevent->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
myevent->PressList->addChild(myset);
            }
            event = new_keyboardevent(&myevent, &(cond->object.cond.FalseList), key, keyascii);
mycond->FalseList->addChild(myevent);
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
dcBezelEvent *myevent1;
            event = new_bezelevent(&myevent1, &(cond->object.cond.TrueList), bezelkey);
mycond->TrueList->addChild(myevent1);
myset = new_setvalue(switchid, 0x0, 0x0, 0x0, offval);
myevent1->PressList->addChild(myset);
            if (transitionid)
            {
myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "-1");
myevent1->PressList->addChild(myset);
            }
dcBezelEvent *myevent2;
            event = new_bezelevent(&myevent2, &(cond->object.cond.FalseList), bezelkey);
mycond->FalseList->addChild(myevent2);
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
dcMouseEvent *mymouse;
        event = new_mouseevent(&mymouse, mySublist, sublist, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
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
dcKeyboardEvent *myevent;
            event = new_keyboardevent(&myevent, sublist, key, keyascii);
mySublist->addChild(myevent);
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
dcBezelEvent *myevent1;
            event = new_bezelevent(&myevent1, sublist, bezelkey);
mySublist->addChild(myevent1);
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
        list1 = new_isequal(&mylist1, &(data->object.cont.SubList), "eq", transitionid, "1");
myitem->addChild(mylist1);
        list2 = new_isequal(&mylist2, &(list1->object.cond.TrueList), "eq", indid, indonval);
mylist1->TrueList->addChild(mylist2);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist2->TrueList->addChild(myset);
        list3 = new_isequal(&mylist3, &(list2->object.cond.FalseList), "eq", switchid, switchonval);
mylist2->FalseList->addChild(mylist3);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist3->FalseList->addChild(myset);

        list4 = new_isequal(&mylist4, &(data->object.cont.SubList), "eq", transitionid, "-1");
myitem->addChild(mylist4);
        list5 = new_isequal(&mylist5, &(list4->object.cond.TrueList), "eq", indid, indonval);
mylist4->TrueList->addChild(mylist5);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist5->FalseList->addChild(myset);
        list6 = new_isequal(&mylist6, &(list5->object.cond.FalseList), "eq", switchid, switchoffval);
mylist5->FalseList->addChild(mylist6);
        myset = new_setvalue(transitionid, 0x0, 0x0, 0x0, "0");
mylist6->TrueList->addChild(myset);
    }

    return data;
}

struct node *new_mouseevent(dcMouseEvent **myitem, dcParent *myparent, struct node **list, const char *x, const char *y, const char *width, const char *height, const char *halign, const char *valign)
{
    struct node *data = add_primitive_node(list);

    *myitem = new dcMouseEvent(myparent);
    (*myitem)->setPosition(x, y);
    (*myitem)->setSize(width, height);
    (*myitem)->setAlignment(halign, valign);

    return data;
}

struct node *new_keyboardevent(dcKeyboardEvent **myitem, struct node **list, const char *key, const char *keyascii)
{
    if (!key && !keyascii) 
    {
        *myitem = 0x0;
        return 0x0;
    }

    struct node *data = add_primitive_node(list);

    if (key)
        *myitem = new dcKeyboardEvent(key[0]);
    else
        *myitem = new dcKeyboardEvent(StrToInt(keyascii, 0));

    return data;
}

struct node *new_bezelevent(dcBezelEvent **myitem, struct node **list, const char *key)
{
    if (!key)
    {
        *myitem = 0x0;
        return 0x0;
    }

    struct node *data = add_primitive_node(list);

    *myitem = new dcBezelEvent(StrToInt(key, 0));

    return data;
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

struct node *new_isequal(dcCondition **myitem, struct node **list, const char *opspec, const char *val1, const char *val2)
{
    const char *onestr = strdup("1");

    if (!val1 && !val2)
    {
        *myitem = 0x0;
        return 0x0;
    }

    struct node *data = add_primitive_node(list);

    int myopspec, datatype1, datatype2;

    myopspec = Simple;
    if (opspec)
    {
        if (!strcasecmp(opspec, "eq")) myopspec = IfEquals;
        else if (!strcasecmp(opspec, "ne")) myopspec = IfNotEquals;
        else if (!strcasecmp(opspec, "gt")) myopspec = IfGreaterThan;
        else if (!strcasecmp(opspec, "lt")) myopspec = IfLessThan;
        else if (!strcasecmp(opspec, "ge")) myopspec = IfGreaterOrEquals;
        else if (!strcasecmp(opspec, "le")) myopspec = IfLessOrEquals;
    }

    datatype1 = get_data_type(val1);
    datatype2 = get_data_type(val2);
    if (datatype1 == UNDEFINED_TYPE && datatype2 == UNDEFINED_TYPE)
    {
        datatype1 = STRING_TYPE;
        datatype2 = STRING_TYPE;
    }
    else if (datatype1 == UNDEFINED_TYPE) datatype1 = datatype2;
    else if (datatype2 == UNDEFINED_TYPE) datatype2 = datatype1;

    if (!val1) val1 = onestr;
    if (!val2) val2 = onestr;

    *myitem = new dcCondition(myopspec, datatype1, getVariablePointer(datatype1, val1), datatype2, getVariablePointer(datatype2, val2));

    return data;
}
