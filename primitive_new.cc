#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "varlist.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "opengl_draw.hh"
#include "PixelStream.hh"

extern void window_init(int, int , int, int, int);
extern void *LoadFont(char *, char *);
extern int LoadTexture(char *);
extern void *LoadConstant(int, void *);

extern appdata AppData;

struct node *new_mouseevent(struct node *, struct node **, char *, char *, char *, char *, char *, char *);
struct node *new_keyboardevent(struct node *, struct node **, char *, char *);
struct node *new_bezelevent(struct node *, struct node **, char *);
struct node *new_setvalue(struct node *, struct node **, char *, char *, char *, char *, const char *);
struct node *new_isequal(struct node *, struct node **, const char *, char *, const char *);

static struct node *add_primitive_node(struct node *, struct node **, Type, char *, char *, const char *, char *, char *, char *, char *);
static void set_geometry(struct node *, char *, char *, const char *, char *, char *, char *, char *);
static void *get_data_pointer(int, const char *, void *);
static int get_data_type(const char *);

static float fzero = 0;
static int izero = 0;
static char emptystr = '\0';


void new_window(int fullscreen, int OriginX, int OriginY, int SizeX, int SizeY)
{
    NewNode(NULL, &(AppData.window));

    window_init(fullscreen, OriginX, OriginY, SizeX, SizeY);
    graphics_init();
}

void new_panels(char *var)
{
    AppData.window->object.win.active_display = (int *)get_data_pointer(INTEGER_TYPE, var, &izero);
}

struct node *new_panel(char *index, char *colorspec, char *vwidth, char *vheight)
{
    struct node *data = NewList(AppData.window);

    data->info.type = Panel;
    data->object.panel.displayID = StrToInt(index, 0);
    data->object.panel.orthoX = StrToFloat(vwidth, 100);
    data->object.panel.orthoY = StrToFloat(vheight, 100);
    data->object.panel.vwidth = StrToFloat(vwidth, 100);
    data->object.panel.vheight = StrToFloat(vheight, 100);
    data->info.w = &(data->object.panel.vwidth);
    data->info.h = &(data->object.panel.vheight);
    data->object.panel.color = StrToColor(colorspec, 0, 0, 0, 1);

    return data;
}

struct node *new_container(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *vwidth, char *vheight, char *rotate)
{
    struct node *data = add_primitive_node(parent, list, Container, x, y, width, height, halign, valign, rotate);

    data->object.cont.vwidth = (float *)get_data_pointer(FLOAT_TYPE, vwidth, data->info.w);
    data->object.cont.vheight = (float *)get_data_pointer(FLOAT_TYPE, vheight, data->info.h);

    return data;
}

struct node *new_vertex(struct node *parent, struct node **list, char *x, char *y)
{
    struct node *data = add_primitive_node(parent, list, Vertex, x, y, NULL, NULL, NULL, NULL, NULL);

    return data;
}

struct node *new_line(struct node *parent, struct node **list, char *linewidth, char *color)
{
    struct node *data = add_primitive_node(parent, list, Line, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    data->object.line.linewidth = StrToFloat(linewidth, 1);
    data->object.line.color = StrToColor(color, 1, 1, 1, 1);

    return data;
}

struct node *new_polygon(struct node *parent, struct node **list, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Polygon, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (fillcolor) data->object.poly.fill = 1;
    if (linecolor || linewidth) data->object.poly.outline = 1;

    data->object.poly.FillColor = StrToColor(fillcolor, 1, 1, 1, 1);
    data->object.poly.LineColor = StrToColor(linecolor, 1, 1, 1, 1);
    data->object.poly.linewidth = StrToFloat(linewidth, 1);

    return data;
}

struct node *new_rectangle(struct node *parent, struct node **list, char *x, char *y, char *width, char *height,
                           char *halign, char *valign, char *rotate, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Rectangle, x, y, width, height, halign, valign, rotate);

    if (fillcolor) data->object.rect.fill = 1;
    if (linecolor || linewidth) data->object.rect.outline = 1;

    data->object.rect.FillColor = StrToColor(fillcolor, 1, 1, 1, 1);
    data->object.rect.LineColor = StrToColor(linecolor, 1, 1, 1, 1);
    data->object.rect.linewidth = StrToFloat(linewidth, 1);

    return data;
}

struct node *new_circle(struct node *parent, struct node **list, char *x, char *y, char *halign, char *valign, char *radius,
                        char *segments, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Circle, x, y, NULL, NULL, halign, valign, NULL);

    if (fillcolor) data->object.circle.fill = 1;
    if (linecolor || linewidth) data->object.circle.outline = 1;

    data->object.circle.radius = (float *)get_data_pointer(FLOAT_TYPE, radius, &fzero);
    data->object.circle.segments = StrToInt(segments, 80);
    data->object.circle.FillColor = StrToColor(fillcolor, 1, 1, 1, 1);
    data->object.circle.LineColor = StrToColor(linecolor, 1, 1, 1, 1);
    data->object.circle.linewidth = StrToFloat(linewidth, 1);

    return data;
}

struct node *new_string(struct node *parent, struct node **list, char *x, char *y, char *rotate, char *fontsize, char *halign, char *valign,
                        char *color, char *bgcolor, char *shadowoffset, char *font, char *face, char *format, char *forcemono, char *value)
{
    struct node *data = add_primitive_node(parent, list, String, x, y, "0", fontsize, halign, valign, rotate);

    data->object.string.fontSize = (float *)get_data_pointer(FLOAT_TYPE, fontsize, &fzero);
    data->object.string.halign = (HAlignment)(data->info.halign);
    data->object.string.valign = (VAlignment)(data->info.valign);

    data->object.string.forcemono = flMonoNone;
    if (forcemono)
    {
        if (!strcmp(forcemono, "Numeric")) data->object.string.forcemono = flMonoNumeric;
        else if (!strcmp(forcemono, "AlphaNumeric")) data->object.string.forcemono = flMonoAlphaNumeric;
        else if (!strcmp(forcemono, "All")) data->object.string.forcemono = flMonoAll;
    }

    data->object.string.color = StrToColor(color, 1, 1, 1, 1);
    if (bgcolor)
    {
        data->object.string.background = 1;
        data->object.string.bgcolor = StrToColor(bgcolor, 0, 0, 0, 1);
    }
    data->object.string.shadowOffset = (float *)get_data_pointer(FLOAT_TYPE, shadowoffset, &fzero);
    data->object.string.fontID = LoadFont(font, face);

    data->object.string.value = get_data_pointer(STRING_TYPE, value, &emptystr);
    data->object.string.datatype = get_data_type(value);
    if (data->object.string.datatype == UNDEFINED_TYPE) data->object.string.datatype = STRING_TYPE;
    if (format != NULL) data->object.string.format = strdup(format);
    else
    {
        switch (data->object.string.datatype)
        {
            case FLOAT_TYPE:
                data->object.string.format = strdup("%.1f");
                break;
            case INTEGER_TYPE:
                data->object.string.format = strdup("%d");
                break;
            case STRING_TYPE:
                data->object.string.format = strdup("%s");
                break;
        }
    }

    return data;
}

struct node *new_image(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate, char *filename)
{
    struct node *data = add_primitive_node(parent, list, Image, x, y, width, height, halign, valign, rotate);

    data->object.image.textureID = LoadTexture(filename);

    return data;
}

struct node *new_pixel_stream(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate, char *protocolstr, char *host, char *port, char *shmemkey, char *filename)
{
    PixelStreamData *mypixelstream = 0x0;
    PixelStreamData *match = 0x0;
    std::list<PixelStreamData *>::iterator psditem;

    unsigned protocol = PixelStreamFileProtocol;
    if (protocolstr)
    {
        if (!strcasecmp(protocolstr, "TCP")) protocol = PixelStreamTcpProtocol;
    }

    PixelStreamFile *myfilepixelstream;
    PixelStreamTcp *mytcppixelstream;

    switch (protocol)
    {
        case PixelStreamFileProtocol:
            myfilepixelstream = new PixelStreamFile;

            if (myfilepixelstream->initialize(filename, StrToInt(shmemkey, 0), 0))
            {
                delete myfilepixelstream;
                return 0x0;
            }

            PixelStreamFile *psdfileitem;
            for (psditem = AppData.pixelstreams.begin(); psditem != AppData.pixelstreams.end(); psditem++)
            {
                if ((*psditem)->protocol == PixelStreamFileProtocol)
                {
                    psdfileitem = (PixelStreamFile *)*psditem;
                    if (!strcmp(myfilepixelstream->getFileName(), psdfileitem->getFileName()) && myfilepixelstream->getShmemKey() == psdfileitem->getShmemKey())
                    {
                        match = *psditem;
                    }
                }
            }

            if (match)
            {
                delete myfilepixelstream;
                mypixelstream = match;
            }
            else
            {
                mypixelstream = (PixelStreamFile *)myfilepixelstream;
                mypixelstream->protocol = PixelStreamFileProtocol;
                AppData.pixelstreams.push_back(mypixelstream);
            }
            break;
        case PixelStreamTcpProtocol:
            mytcppixelstream = new PixelStreamTcp;

            if (mytcppixelstream->readerInitialize(host, StrToInt(port, 0)))
            {
                delete mytcppixelstream;
                return 0x0;
            }

            PixelStreamTcp *psdtcpitem;
            for (psditem = AppData.pixelstreams.begin(); psditem != AppData.pixelstreams.end(); psditem++)
            {
                if ((*psditem)->protocol == PixelStreamTcpProtocol)
                {
                    psdtcpitem = (PixelStreamTcp *)*psditem;
                    if (!strcmp(mytcppixelstream->getHost(), psdtcpitem->getHost()) && mytcppixelstream->getPort() == psdtcpitem->getPort())
                    {
                        match = *psditem;
                    }
                }
            }

            if (match)
            {
                delete mytcppixelstream;
                mypixelstream = match;
            }
            else
            {
                mypixelstream = (PixelStreamData *)mytcppixelstream;
                mypixelstream->protocol = PixelStreamTcpProtocol;
                AppData.pixelstreams.push_back(mypixelstream);
            }
            break;
        default:
            return 0x0;
    }

    struct node *data = add_primitive_node(parent, list, PixelStreamView, x, y, width, height, halign, valign, rotate);

    init_texture(&data->object.pixelstreamview.textureID);
    data->object.pixelstreamview.psd = mypixelstream;
    data->object.pixelstreamview.pixels = 0x0;
    data->object.pixelstreamview.memallocation = 0;

    return data;
}

struct node *new_button(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign,
                        char *rotate, char *type, char *switchid, char *switchonval, char *switchoffval, char *indid, char *indonval,
                        char *activevar, char *activeval, char *transitionid, char *key, char *keyascii, char *bezelkey)
{
    int toggle=0, momentary=0;
    struct node *cond, *event, *curlist, **sublist, *list1, *list2, *list3, *list4, *list5, *list6;
    char *offval, *zerostr=strdup("0"), *onestr=strdup("1");
    struct node *data = add_primitive_node(parent, list, Container, x, y, width, height, halign, valign, rotate);

    if (type)
    {
        if (!strcmp(type, "Toggle")) toggle = 1;
        if (!strcmp(type, "Momentary")) momentary = 1;
    }

    data->object.cont.vwidth = data->info.w;
    data->object.cont.vheight = data->info.h;

    curlist = data;
    sublist = &(data->object.cont.SubList);
    if (activevar)
    {
        if (!activeval) activeval = onestr;
        curlist = new_isequal(data, &(data->object.cont.SubList), "eq", activevar, activeval);
        sublist = &(curlist->object.cond.TrueList);
    }

    if (!switchonval) switchonval = onestr;
    if (toggle || momentary)
    {
        if (switchoffval) offval = switchoffval;
        else offval = zerostr;
    }
    else offval = NULL;

    if (toggle)
    {
        cond = new_isequal(curlist, sublist, "eq", indid, indonval);
        event = new_mouseevent(cond, &(cond->object.cond.TrueList), NULL, NULL, NULL, NULL, NULL, NULL);
        new_setvalue(event, &(event->object.me.PressList), switchid, NULL, NULL, NULL, offval);
        if (transitionid) new_setvalue(event, &(event->object.me.PressList), transitionid, NULL, NULL, NULL, "-1");
        event = new_mouseevent(cond, &(cond->object.cond.FalseList), NULL, NULL, NULL, NULL, NULL, NULL);
        new_setvalue(event, &(event->object.me.PressList), switchid, NULL, NULL, NULL, switchonval);
        if (transitionid) new_setvalue(event, &(event->object.me.PressList), transitionid, NULL, NULL, NULL, "1");
        if (key || keyascii)
        {
            event = new_keyboardevent(cond, &(cond->object.cond.TrueList), key, keyascii);
            new_setvalue(event, &(event->object.ke.PressList), switchid, NULL, NULL, NULL, offval);
            if (transitionid) new_setvalue(event, &(event->object.ke.PressList), transitionid, NULL, NULL, NULL, "-1");
            event = new_keyboardevent(cond, &(cond->object.cond.FalseList), key, keyascii);
            new_setvalue(event, &(event->object.ke.PressList), switchid, NULL, NULL, NULL, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.ke.PressList), transitionid, NULL, NULL, NULL, "1");
        }
        if (bezelkey)
        {
            event = new_bezelevent(cond, &(cond->object.cond.TrueList), bezelkey);
            new_setvalue(event, &(event->object.be.PressList), switchid, NULL, NULL, NULL, offval);
            if (transitionid) new_setvalue(event, &(event->object.be.PressList), transitionid, NULL, NULL, NULL, "-1");
            event = new_bezelevent(cond, &(cond->object.cond.FalseList), bezelkey);
            new_setvalue(event, &(event->object.be.PressList), switchid, NULL, NULL, NULL, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.be.PressList), transitionid, NULL, NULL, NULL, "1");
        }
    }
    else
    {
        event = new_mouseevent(curlist, sublist, NULL, NULL, NULL, NULL, NULL, NULL);
        new_setvalue(event, &(event->object.me.PressList), switchid, NULL, NULL, NULL, switchonval);
        if (transitionid) new_setvalue(event, &(event->object.me.PressList), transitionid, NULL, NULL, NULL, "1");
        if (momentary)
        {
            new_setvalue(event, &(event->object.me.ReleaseList), switchid, NULL, NULL, NULL, offval);
            if (transitionid) new_setvalue(event, &(event->object.me.ReleaseList), transitionid, NULL, NULL, NULL, "-1");
        }
        if (key || keyascii)
        {
            event = new_keyboardevent(curlist, sublist, key, keyascii);
            new_setvalue(event, &(event->object.ke.PressList), switchid, NULL, NULL, NULL, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.ke.PressList), transitionid, NULL, NULL, NULL, "1");
            if (momentary)
            {
                new_setvalue(event, &(event->object.ke.ReleaseList), switchid, NULL, NULL, NULL, offval);
                if (transitionid) new_setvalue(event, &(event->object.ke.ReleaseList), transitionid, NULL, NULL, NULL, "-1");
            }
        }
        if (bezelkey)
        {
            event = new_bezelevent(curlist, sublist, bezelkey);
            new_setvalue(event, &(event->object.be.PressList), switchid, NULL, NULL, NULL, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.be.PressList), transitionid, NULL, NULL, NULL, "1");
            if (momentary)
            {
                new_setvalue(event, &(event->object.be.ReleaseList), switchid, NULL, NULL, NULL, offval);
                if (transitionid) new_setvalue(event, &(event->object.be.ReleaseList), transitionid, NULL, NULL, NULL, "-1");
            }
        }
    }

    if (transitionid)
    {
        list1 = new_isequal(data, &(data->object.cont.SubList), "eq", transitionid, "1");
        list2 = new_isequal(list1, &(list1->object.cond.TrueList), "eq", indid, indonval);
        new_setvalue(list2, &(list2->object.cond.TrueList), transitionid, NULL, NULL, NULL, "0");
        list3 = new_isequal(list2, &(list2->object.cond.FalseList), "eq", switchid, switchonval);
        new_setvalue(list3, &(list3->object.cond.FalseList), transitionid, NULL, NULL, NULL, "0");

        list4 = new_isequal(data, &(data->object.cont.SubList), "eq", transitionid, "-1");
        list5 = new_isequal(list4, &(list4->object.cond.TrueList), "eq", indid, indonval);
        new_setvalue(list5, &(list5->object.cond.FalseList), transitionid, NULL, NULL, NULL, "0");
        list6 = new_isequal(list5, &(list5->object.cond.FalseList), "eq", switchid, switchoffval);
        new_setvalue(list6, &(list6->object.cond.TrueList), transitionid, NULL, NULL, NULL, "0");
    }

    return data;
}

struct node *new_adi(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *outerradius, char *ballradius,
                     char *chevronwidth, char *chevronheight, char *ballimage_filename, char *coverimage_filename,
                     char *roll, char *pitch, char *yaw, char *roll_error, char *pitch_error, char *yaw_error)
{
    float defouterrad, defballrad, defchevron;
    struct node *data = add_primitive_node(parent, list, ADI, x, y, width, height, halign, valign, NULL);

    if (ballimage_filename) data->object.adi.ballID = LoadTexture(ballimage_filename);
    if (coverimage_filename) data->object.adi.bkgdID = LoadTexture(coverimage_filename);

    defouterrad = 0.5 * (fminf(*(data->info.w), *(data->info.h)));
    defballrad = 0.9 * defouterrad;
    defchevron = 0.2 * defouterrad;

    data->object.adi.outerradius = (float *)get_data_pointer(FLOAT_TYPE, outerradius, &defouterrad);
    data->object.adi.ballradius = (float *)get_data_pointer(FLOAT_TYPE, ballradius, &defballrad);
    data->object.adi.chevronW = (float *)get_data_pointer(FLOAT_TYPE, chevronwidth, &defchevron);
    data->object.adi.chevronH = (float *)get_data_pointer(FLOAT_TYPE, chevronheight, &defchevron);

    data->object.adi.roll = (float *)get_data_pointer(FLOAT_TYPE, roll, &fzero);
    data->object.adi.pitch = (float *)get_data_pointer(FLOAT_TYPE, pitch, &fzero);
    data->object.adi.yaw = (float *)get_data_pointer(FLOAT_TYPE, yaw, &fzero);

    data->object.adi.rollError = (float *)get_data_pointer(FLOAT_TYPE, roll_error, &fzero);
    data->object.adi.pitchError = (float *)get_data_pointer(FLOAT_TYPE, pitch_error, &fzero);
    data->object.adi.yawError = (float *)get_data_pointer(FLOAT_TYPE, yaw_error, &fzero);

    return data;
}

struct node *new_mouseevent(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign)
{
    struct node *data = add_primitive_node(parent, list, MouseEvent, x, y, width, height, halign, valign, NULL);

    return data;
}

struct node *new_keyboardevent(struct node *parent, struct node **list, char *key, char *keyascii)
{
    if (!key && !keyascii) return NULL;

    struct node *data = add_primitive_node(parent, list, KeyboardEvent, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (key)
        data->object.ke.key = key[0];
    else
        data->object.ke.key = StrToInt(keyascii, 0);

    return data;
}

struct node *new_bezelevent(struct node *parent, struct node **list, char *key)
{
    if (!key) return NULL;

    struct node *data = add_primitive_node(parent, list, BezelEvent, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    data->object.be.key = StrToInt(key, 0);

    return data;
}

struct ModifyValue get_setvalue_data(char *varspec, char *opspec, char *minspec, char *maxspec, const char *valspec)
{
    struct ModifyValue ret;

    ret.optype = Equals;
    ret.datatype1 = get_data_type(varspec);
    ret.datatype2 = get_data_type(valspec);
    ret.mindatatype = 0;
    ret.maxdatatype = 0;
    ret.var = NULL;
    ret.val = NULL;
    ret.min = NULL;
    ret.max = NULL;

    if (ret.datatype1 != UNDEFINED_TYPE)
    {
        if (!opspec) ret.optype = Equals;
        else if (!strcmp(opspec, "+=")) ret.optype = PlusEquals;
        else if (!strcmp(opspec, "-=")) ret.optype = MinusEquals;

        ret.var = get_data_pointer(ret.datatype1, varspec, NULL);

        if (ret.datatype2 == UNDEFINED_TYPE) ret.datatype2 = ret.datatype1;
        switch (ret.datatype2)
        {
            case FLOAT_TYPE:
                ret.val = get_data_pointer(FLOAT_TYPE, valspec, &fzero);
                break;
            case INTEGER_TYPE:
                ret.val = get_data_pointer(INTEGER_TYPE, valspec, &izero);
                break;
            case STRING_TYPE:
                ret.val = get_data_pointer(STRING_TYPE, valspec, &emptystr);
                break;
        }

        if (minspec)
        {
            ret.mindatatype = get_data_type(minspec);
            if (ret.mindatatype == UNDEFINED_TYPE) ret.mindatatype = ret.datatype1;
            switch (ret.mindatatype)
            {
                case FLOAT_TYPE:
                    ret.min = get_data_pointer(FLOAT_TYPE, minspec, &fzero);
                    break;
                case INTEGER_TYPE:
                    ret.min = get_data_pointer(INTEGER_TYPE, minspec, &izero);
                    break;
                case STRING_TYPE:
                    ret.min = get_data_pointer(STRING_TYPE, minspec, &emptystr);
                    break;
            }
        }

        if (maxspec)
        {
            ret.maxdatatype = get_data_type(maxspec);
            if (ret.maxdatatype == UNDEFINED_TYPE) ret.maxdatatype = ret.datatype1;
            switch (ret.maxdatatype)
            {
                case FLOAT_TYPE:
                    ret.max = get_data_pointer(FLOAT_TYPE, maxspec, &fzero);
                    break;
                case INTEGER_TYPE:
                    ret.max = get_data_pointer(INTEGER_TYPE, maxspec, &izero);
                    break;
                case STRING_TYPE:
                    ret.max = get_data_pointer(STRING_TYPE, maxspec, &emptystr);
                    break;
            }
        }
    }
    return ret;
}

struct node *new_animation(struct node *parent, struct node **list, char *duration)
{
    struct node *data = add_primitive_node(parent, list, Animate, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

    data->object.anim.duration = StrToFloat(duration, 1);;

    return data;
}

struct node *new_setvalue(struct node *parent, struct node **list, char *var, char *optype, char *min, char *max, const char *val)
{
    struct ModifyValue myset = get_setvalue_data(var, optype, min, max, val);

    if (myset.datatype1 == UNDEFINED_TYPE) return NULL;

    struct node *data = add_primitive_node(parent, list, SetValue, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    data->object.modval.optype = myset.optype;    
    data->object.modval.datatype1 = myset.datatype1;    
    data->object.modval.datatype2 = myset.datatype2;    
    data->object.modval.mindatatype = myset.mindatatype;    
    data->object.modval.maxdatatype = myset.maxdatatype;    
    data->object.modval.var = myset.var;    
    data->object.modval.val = myset.val;    
    data->object.modval.min = myset.min;    
    data->object.modval.max = myset.max;    

    return data;    
}

struct node *new_isequal(struct node *parent, struct node **list, const char *opspec, char *val1, const char *val2)
{
    char *onestr=strdup("1");

    if (!val1 && !val2) return NULL;

    struct node *data = add_primitive_node(parent, list, Condition, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (parent) data->info.w = parent->info.w;
    if (parent) data->info.h = parent->info.h;

    data->object.cond.opspec = Simple;
    if (opspec)
    {
        if (!strcasecmp(opspec, "eq")) data->object.cond.opspec = IfEquals;
        else if (!strcasecmp(opspec, "ne")) data->object.cond.opspec = IfNotEquals;
        else if (!strcasecmp(opspec, "gt")) data->object.cond.opspec = IfGreaterThan;
        else if (!strcasecmp(opspec, "lt")) data->object.cond.opspec = IfLessThan;
        else if (!strcasecmp(opspec, "ge")) data->object.cond.opspec = IfGreaterOrEquals;
        else if (!strcasecmp(opspec, "le")) data->object.cond.opspec = IfLessOrEquals;
    }

    data->object.cond.datatype1 = get_data_type(val1);
    data->object.cond.datatype2 = get_data_type(val2);
    if (data->object.cond.datatype1 == UNDEFINED_TYPE && data->object.cond.datatype2 == UNDEFINED_TYPE)
    {
        data->object.cond.datatype1 = STRING_TYPE;
        data->object.cond.datatype2 = STRING_TYPE;
    }
    else if (data->object.cond.datatype1 == UNDEFINED_TYPE)
        data->object.cond.datatype1 = data->object.cond.datatype2;
    else if (data->object.cond.datatype2 == UNDEFINED_TYPE)
        data->object.cond.datatype2 = data->object.cond.datatype1;

    if (!val1) val1 = onestr;
    if (!val2) val2 = onestr;

    data->object.cond.val1 = get_data_pointer(data->object.cond.datatype1, val1, NULL);
    data->object.cond.val2 = get_data_pointer(data->object.cond.datatype2, val2, NULL);

    return data;
}

static struct node *add_primitive_node(struct node *parent, struct node **list, Type type, char *x, char *y, const char *width, char *height, char *halign, char *valign, char *rotate)
{
    struct node *data = NewNode(parent, list);
    data->info.type = type;
    set_geometry(data, x, y, width, height, halign, valign, rotate);
    return data;
}

static void set_geometry(struct node *data, char *x, char *y, const char *width, char *height, char *halign, char *valign, char *rotate)
{
    // it's important to pass container width and height down through all primitives in the tree, even those
    // that don't seem to be geometric, so that the size of contained elements can be calculated at draw time.
    if (data->p_parent->info.type == Container)
    {
        data->info.containerW = data->p_parent->object.cont.vwidth;
        data->info.containerH = data->p_parent->object.cont.vheight;
    }
    else
    {
        data->info.containerW = data->p_parent->info.w;
        data->info.containerH = data->p_parent->info.h;
    }

    // set x and y pointers to NULL if those values aren't defined. This tells the draw-time logic that x and y
    // values need to be calculated.
    if (x) data->info.x = (float *)get_data_pointer(FLOAT_TYPE, x, &fzero);
    else data->info.x = 0;
    if (y) data->info.y = (float *)get_data_pointer(FLOAT_TYPE, y, &fzero);
    else data->info.y = 0;

    data->info.halign = StrToHAlign(halign, AlignLeft);
    data->info.valign = StrToVAlign(valign, AlignBottom);
    data->info.w = (float *)get_data_pointer(FLOAT_TYPE, width, data->info.containerW);
    data->info.h = (float *)get_data_pointer(FLOAT_TYPE, height, data->info.containerH);
    data->info.rotate = (float *)get_data_pointer(FLOAT_TYPE, rotate, &fzero);
}

int check_dynamic_element(const char *spec)
{
    if (spec)
    {
        if (strlen(spec) > 1)
        {
            if (spec[0] == '@') return 1;
        }
    }
    return 0;
}

static void *get_data_pointer(int type, const char *valstr, void *defval)
{
    float fval;
    int ival;
    char *sval;
    const char *inptr;

    if (check_dynamic_element(valstr)) return get_pointer(&valstr[1]);

    switch (type)
    {
        case FLOAT_TYPE:
            if (defval)
                fval = StrToFloat(valstr, *(float *)defval);
            else
                fval = StrToFloat(valstr, 0);
            return LoadConstant(type, &fval);
        case INTEGER_TYPE:
            if (defval)
                ival = StrToInt(valstr, *(int *)defval);
            else
                ival = StrToInt(valstr, 0);
            return LoadConstant(type, &ival);
        case STRING_TYPE:
            inptr = valstr;
            if (valstr)
            {
                if (valstr[0] == '\\') inptr = &valstr[1];
            }
            if (defval)
                sval = StrToStr(inptr, (char *)defval);
            else
                sval = StrToStr(inptr, &emptystr);
            return LoadConstant(type, sval);
        default:
            return 0;
    }
}

static int get_data_type(const char *valstr)
{
    if (check_dynamic_element(valstr)) return get_datatype(&valstr[1]);
    return UNDEFINED_TYPE;
}
