#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <strings.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "utils/tidy.hh"
#include "utils/msg.hh"
#include "PixelStream/PixelStream.hh"
#include "dc.hh"
#include "varlist.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "opengl_draw.hh"
#include "alignment.hh"

extern void window_init(bool, int , int, int, int);

extern appdata AppData;

struct node *new_mouseevent(struct node *, struct node **, char *, char *, char *, char *, char *, char *);
struct node *new_keyboardevent(struct node *, struct node **, char *, char *);
struct node *new_bezelevent(struct node *, struct node **, char *);
struct node *new_setvalue(struct node *, struct node **, char *, char *, char *, char *, const char *);
struct node *new_isequal(struct node *, struct node **, const char *, char *, const char *);

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

static float *getFloatPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (float *)get_pointer(&valstr[1]);
    else return dcLoadConstant(StrToFloat(valstr, 0));
}

static float *getFloatPointer(const char *valstr, float defval)
{
    if (check_dynamic_element(valstr)) return (float *)get_pointer(&valstr[1]);
    else return dcLoadConstant(StrToFloat(valstr, defval));
}

static int *getIntegerPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (int *)get_pointer(&valstr[1]);
    else return dcLoadConstant(StrToInt(valstr, 0));
}

static char *getStringPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (char *)get_pointer(&valstr[1]);
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

static int get_data_type(const char *valstr)
{
    if (check_dynamic_element(valstr)) return get_datatype(&valstr[1]);
    return UNDEFINED_TYPE;
}

static struct node *add_primitive_node(struct node *parent, struct node **list, Type type, char *x, char *y, const char *width, char *height, char *halign, char *valign, char *rotate)
{
    struct node *data = (struct node *)calloc(1, sizeof(struct node));

    if (*list == 0x0)
        *list = data;
    else
        (*list)->p_tail->p_next = data;

    (*list)->p_tail = data;
    data->p_next = 0x0;
    data->p_parent = parent;

    data->info.type = type;

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

    // set x and y pointers to 0x0 if those values aren't defined. This tells the draw-time logic that x and y
    // values need to be calculated.
    if (x) data->info.x = getFloatPointer(x);
    else data->info.x = 0x0;
    if (y) data->info.y = getFloatPointer(y);
    else data->info.y = 0x0;

    data->info.halign = StrToHAlign(halign, AlignLeft);
    data->info.valign = StrToVAlign(valign, AlignBottom);
    data->info.w = getFloatPointer(width, *(data->info.containerW));
    data->info.h = getFloatPointer(height, *(data->info.containerH));
    data->info.rotate = getFloatPointer(rotate);

    return data;
}

void new_window(bool fullscreen, int OriginX, int OriginY, int SizeX, int SizeY, char *activedisp)
{
    window_init(fullscreen, OriginX, OriginY, SizeX, SizeY);
    graphics_init();
    AppData.window.active_display = getIntegerPointer(activedisp);
}

struct node *new_panel(char *index, char *colorspec, char *vwidth, char *vheight)
{
    struct node *data = (struct node *)calloc(1, sizeof(struct node));

    data->object.panel.displayID = StrToInt(index, 0);
    data->object.panel.orthoX = StrToFloat(vwidth, 100);
    data->object.panel.orthoY = StrToFloat(vheight, 100);
    data->object.panel.vwidth = StrToFloat(vwidth, 100);
    data->object.panel.vheight = StrToFloat(vheight, 100);
    data->info.w = &(data->object.panel.vwidth);
    data->info.h = &(data->object.panel.vheight);
    data->object.panel.color = StrToColor(colorspec, 0, 0, 0, 1);

    AppData.window.panels.push_back(data);
    AppData.window.current_panel = data;

    return data;
}

struct node *new_container(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *vwidth, char *vheight, char *rotate)
{
    struct node *data = add_primitive_node(parent, list, Container, x, y, width, height, halign, valign, rotate);

    data->object.cont.vwidth = getFloatPointer(vwidth, *(data->info.w));
    data->object.cont.vheight = getFloatPointer(vheight, *(data->info.h));

    return data;
}

struct node *new_vertex(struct node *parent, struct node **list, char *x, char *y)
{
    struct node *data = add_primitive_node(parent, list, Vertex, x, y, 0x0, 0x0, 0x0, 0x0, 0x0);

    return data;
}

struct node *new_line(struct node *parent, struct node **list, char *linewidth, char *color)
{
    struct node *data = add_primitive_node(parent, list, Line, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

    data->object.line.linewidth = StrToFloat(linewidth, 1);
    data->object.line.color = StrToColor(color, 1, 1, 1, 1);

    return data;
}

struct node *new_polygon(struct node *parent, struct node **list, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Polygon, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

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
    struct node *data = add_primitive_node(parent, list, Circle, x, y, 0x0, 0x0, halign, valign, 0x0);

    if (fillcolor) data->object.circle.fill = 1;
    if (linecolor || linewidth) data->object.circle.outline = 1;

    data->object.circle.radius = getFloatPointer(radius);
    data->object.circle.segments = StrToInt(segments, 80);
    data->object.circle.FillColor = StrToColor(fillcolor, 1, 1, 1, 1);
    data->object.circle.LineColor = StrToColor(linecolor, 1, 1, 1, 1);
    data->object.circle.linewidth = StrToFloat(linewidth, 1);

    return data;
}

// TODO: This parsing should be simplified and/or combined with string parsing in xml_stringsub.cc
static size_t parse_var(std::vector<VarString *> *vstring, std::string mystr)
{
    size_t var_start, var_end;
    size_t fmt_start = mystr.find('(');
    size_t fmt_end = mystr.find(')');
    bool braced;
    std::string varstr = "@";

    if (mystr[1] == '{') 
    {
        braced = true;
        var_start = 2;
    }
    else
    {
        braced = false;
        var_start = 1;
    }

    if (fmt_start != std::string::npos) var_end = fmt_start;
    else if (braced) var_end = mystr.find('}');
    else var_end = mystr.find(' ');

    if (var_end == std::string::npos) varstr += mystr.substr(var_start, std::string::npos);
    else varstr += mystr.substr(var_start, var_end - var_start);

    if (fmt_start != std::string::npos && fmt_end != std::string::npos)
        vstring->push_back(new VarString(get_data_type(varstr.c_str()), getStringPointer(varstr.c_str()), mystr.substr(fmt_start+1, fmt_end-fmt_start-1).c_str()));
    else
        vstring->push_back(new VarString(get_data_type(varstr.c_str()), getStringPointer(varstr.c_str()), 0x0));

    if (braced) return mystr.find('}') + 1;
    else if (fmt_end != std::string::npos) return fmt_end + 1;
    else return mystr.find(' ');
}

static void parse_string(std::vector<VarString *> *vstring, std::vector<std::string> *filler, std::string mystr)
{
    size_t vstart, vlen, curpos = 0;

    do
    {
        vstart = mystr.find('@', curpos);
        filler->push_back(mystr.substr(curpos, vstart-curpos));
        if (vstart == std::string::npos) return;
        vlen = parse_var(vstring, mystr.substr(vstart, std::string::npos));
        if (vlen == std::string::npos)
        {
            filler->push_back("");
            return;
        }
        curpos = vstart + vlen;
    } while (curpos < mystr.size());

    filler->push_back("");
}

struct node *new_string(struct node *parent, struct node **list, char *x, char *y, char *rotate, char *fontsize, char *halign, char *valign,
                        char *color, char *bgcolor, char *shadowoffset, char *font, char *face, char *forcemono, char *value)
{
    struct node *data = add_primitive_node(parent, list, String, x, y, "0", fontsize, halign, valign, rotate);

    data->object.string.fontSize = getFloatPointer(fontsize);
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
        data->object.string.background = true;
        data->object.string.bgcolor = StrToColor(bgcolor, 0, 0, 0, 1);
    }
    else
        data->object.string.background = false;
    data->object.string.shadowOffset = getFloatPointer(shadowoffset);
    data->object.string.fontID = dcLoadFont(font, face);

    parse_string(&(data->vstring), &(data->filler), value);

    return data;
}

struct node *new_image(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate, char *filename)
{
    struct node *data = add_primitive_node(parent, list, Image, x, y, width, height, halign, valign, rotate);

    data->object.image.textureID = dcLoadTexture(filename);

    return data;
}

struct node *new_pixel_stream(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate, char *protocolstr, char *host, char *port, char *shmemkey, char *filename)
{
    PixelStreamData *mypsd = 0x0;
    PixelStreamFile *psf;
    PixelStreamMjpeg *psm;
    PixelStreamTcp *pst;
    PixelStreamItem *mypixelstream = 0x0;
    PixelStreamItem *match = 0x0;
    std::list<PixelStreamItem *>::iterator psitem;

    unsigned protocol = PixelStreamFileProtocol;
    if (protocolstr)
    {
        if (!strcasecmp(protocolstr, "MJPEG")) protocol = PixelStreamMjpegProtocol;
        if (!strcasecmp(protocolstr, "TCP")) protocol = PixelStreamTcpProtocol;
    }

    switch (protocol)
    {
        case PixelStreamFileProtocol:
            psf = new PixelStreamFile;
            if (psf->initialize(filename, StrToInt(shmemkey, 0), PixelStreamReaderFunction))
            {
                delete psf;
                return 0x0;
            }
            mypsd = (PixelStreamData *)psf;
            break;
        case PixelStreamMjpegProtocol:
            psm = new PixelStreamMjpeg;
            if (psm->readerInitialize(host, StrToInt(port, 8080)))
            {
                delete psm;
                return 0x0;
            }
            mypsd = (PixelStreamData *)psm;
            break;
        case PixelStreamTcpProtocol:
            pst = new PixelStreamTcp;
            if (pst->readerInitialize(host, StrToInt(port, 0)))
            {
                delete pst;
                return 0x0;
            }
            mypsd = (PixelStreamData *)pst;
            break;
        default:
            break;
    }

    if (!mypsd) return 0x0;

    for (psitem = AppData.pixelstreams.begin(); psitem != AppData.pixelstreams.end() && !match; psitem++)
    {
        if (*(*psitem)->psd == *mypsd) match = *psitem;
    }

    if (match)
    {
        delete mypsd;
        mypixelstream = match;
    }
    else
    {
        mypixelstream = new PixelStreamItem;
        mypixelstream->psd = (PixelStreamData *)mypsd;
        AppData.pixelstreams.push_back(mypixelstream);
    }

    struct node *data = add_primitive_node(parent, list, PixelStreamView, x, y, width, height, halign, valign, rotate);

    init_texture(&(data->object.pixelstreamview.textureID));
    data->object.pixelstreamview.psi = mypixelstream;
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
    else offval = 0x0;

    if (toggle)
    {
        cond = new_isequal(curlist, sublist, "eq", indid, indonval);
        event = new_mouseevent(cond, &(cond->object.cond.TrueList), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
        new_setvalue(event, &(event->object.me.PressList), switchid, 0x0, 0x0, 0x0, offval);
        if (transitionid) new_setvalue(event, &(event->object.me.PressList), transitionid, 0x0, 0x0, 0x0, "-1");
        event = new_mouseevent(cond, &(cond->object.cond.FalseList), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
        new_setvalue(event, &(event->object.me.PressList), switchid, 0x0, 0x0, 0x0, switchonval);
        if (transitionid) new_setvalue(event, &(event->object.me.PressList), transitionid, 0x0, 0x0, 0x0, "1");
        if (key || keyascii)
        {
            event = new_keyboardevent(cond, &(cond->object.cond.TrueList), key, keyascii);
            new_setvalue(event, &(event->object.ke.PressList), switchid, 0x0, 0x0, 0x0, offval);
            if (transitionid) new_setvalue(event, &(event->object.ke.PressList), transitionid, 0x0, 0x0, 0x0, "-1");
            event = new_keyboardevent(cond, &(cond->object.cond.FalseList), key, keyascii);
            new_setvalue(event, &(event->object.ke.PressList), switchid, 0x0, 0x0, 0x0, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.ke.PressList), transitionid, 0x0, 0x0, 0x0, "1");
        }
        if (bezelkey)
        {
            event = new_bezelevent(cond, &(cond->object.cond.TrueList), bezelkey);
            new_setvalue(event, &(event->object.be.PressList), switchid, 0x0, 0x0, 0x0, offval);
            if (transitionid) new_setvalue(event, &(event->object.be.PressList), transitionid, 0x0, 0x0, 0x0, "-1");
            event = new_bezelevent(cond, &(cond->object.cond.FalseList), bezelkey);
            new_setvalue(event, &(event->object.be.PressList), switchid, 0x0, 0x0, 0x0, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.be.PressList), transitionid, 0x0, 0x0, 0x0, "1");
        }
    }
    else
    {
        event = new_mouseevent(curlist, sublist, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
        new_setvalue(event, &(event->object.me.PressList), switchid, 0x0, 0x0, 0x0, switchonval);
        if (transitionid) new_setvalue(event, &(event->object.me.PressList), transitionid, 0x0, 0x0, 0x0, "1");
        if (momentary)
        {
            new_setvalue(event, &(event->object.me.ReleaseList), switchid, 0x0, 0x0, 0x0, offval);
            if (transitionid) new_setvalue(event, &(event->object.me.ReleaseList), transitionid, 0x0, 0x0, 0x0, "-1");
        }
        if (key || keyascii)
        {
            event = new_keyboardevent(curlist, sublist, key, keyascii);
            new_setvalue(event, &(event->object.ke.PressList), switchid, 0x0, 0x0, 0x0, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.ke.PressList), transitionid, 0x0, 0x0, 0x0, "1");
            if (momentary)
            {
                new_setvalue(event, &(event->object.ke.ReleaseList), switchid, 0x0, 0x0, 0x0, offval);
                if (transitionid) new_setvalue(event, &(event->object.ke.ReleaseList), transitionid, 0x0, 0x0, 0x0, "-1");
            }
        }
        if (bezelkey)
        {
            event = new_bezelevent(curlist, sublist, bezelkey);
            new_setvalue(event, &(event->object.be.PressList), switchid, 0x0, 0x0, 0x0, switchonval);
            if (transitionid) new_setvalue(event, &(event->object.be.PressList), transitionid, 0x0, 0x0, 0x0, "1");
            if (momentary)
            {
                new_setvalue(event, &(event->object.be.ReleaseList), switchid, 0x0, 0x0, 0x0, offval);
                if (transitionid) new_setvalue(event, &(event->object.be.ReleaseList), transitionid, 0x0, 0x0, 0x0, "-1");
            }
        }
    }

    if (transitionid)
    {
        list1 = new_isequal(data, &(data->object.cont.SubList), "eq", transitionid, "1");
        list2 = new_isequal(list1, &(list1->object.cond.TrueList), "eq", indid, indonval);
        new_setvalue(list2, &(list2->object.cond.TrueList), transitionid, 0x0, 0x0, 0x0, "0");
        list3 = new_isequal(list2, &(list2->object.cond.FalseList), "eq", switchid, switchonval);
        new_setvalue(list3, &(list3->object.cond.FalseList), transitionid, 0x0, 0x0, 0x0, "0");

        list4 = new_isequal(data, &(data->object.cont.SubList), "eq", transitionid, "-1");
        list5 = new_isequal(list4, &(list4->object.cond.TrueList), "eq", indid, indonval);
        new_setvalue(list5, &(list5->object.cond.FalseList), transitionid, 0x0, 0x0, 0x0, "0");
        list6 = new_isequal(list5, &(list5->object.cond.FalseList), "eq", switchid, switchoffval);
        new_setvalue(list6, &(list6->object.cond.TrueList), transitionid, 0x0, 0x0, 0x0, "0");
    }

    return data;
}

struct node *new_adi(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *outerradius, char *ballradius,
                     char *chevronwidth, char *chevronheight, char *ballimage_filename, char *coverimage_filename,
                     char *roll, char *pitch, char *yaw, char *roll_error, char *pitch_error, char *yaw_error)
{
    float defouterrad, defballrad, defchevron;
    struct node *data = add_primitive_node(parent, list, ADI, x, y, width, height, halign, valign, 0x0);

    if (ballimage_filename) data->object.adi.ballID = dcLoadTexture(ballimage_filename);
    if (coverimage_filename) data->object.adi.bkgdID = dcLoadTexture(coverimage_filename);

    defouterrad = 0.5 * (fminf(*(data->info.w), *(data->info.h)));
    defballrad = 0.9 * defouterrad;
    defchevron = 0.2 * defouterrad;

    data->object.adi.outerradius = getFloatPointer(outerradius, defouterrad);
    data->object.adi.ballradius = getFloatPointer(ballradius, defballrad);
    data->object.adi.chevronW = getFloatPointer(chevronwidth, defchevron);
    data->object.adi.chevronH = getFloatPointer(chevronheight, defchevron);

    data->object.adi.roll = getFloatPointer(roll);
    data->object.adi.pitch = getFloatPointer(pitch);
    data->object.adi.yaw = getFloatPointer(yaw);

    data->object.adi.rollError = getFloatPointer(roll_error);
    data->object.adi.pitchError = getFloatPointer(pitch_error);
    data->object.adi.yawError = getFloatPointer(yaw_error);

    return data;
}

struct node *new_mouseevent(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign)
{
    struct node *data = add_primitive_node(parent, list, MouseEvent, x, y, width, height, halign, valign, 0x0);

    return data;
}

struct node *new_keyboardevent(struct node *parent, struct node **list, char *key, char *keyascii)
{
    if (!key && !keyascii) return 0x0;

    struct node *data = add_primitive_node(parent, list, KeyboardEvent, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

    if (key)
        data->object.ke.key = key[0];
    else
        data->object.ke.key = StrToInt(keyascii, 0);

    return data;
}

struct node *new_bezelevent(struct node *parent, struct node **list, char *key)
{
    if (!key) return 0x0;

    struct node *data = add_primitive_node(parent, list, BezelEvent, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

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

struct node *new_animation(struct node *parent, struct node **list, char *duration)
{
    struct node *data = add_primitive_node(parent, list, Animate, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

    data->object.anim.duration = StrToFloat(duration, 1);;

    return data;
}

struct node *new_setvalue(struct node *parent, struct node **list, char *var, char *optype, char *min, char *max, const char *val)
{
    struct ModifyValue myset = get_setvalue_data(var, optype, min, max, val);

    if (myset.datatype1 == UNDEFINED_TYPE) return 0x0;

    struct node *data = add_primitive_node(parent, list, SetValue, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

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

    if (!val1 && !val2) return 0x0;

    struct node *data = add_primitive_node(parent, list, Condition, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

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

    data->object.cond.val1 = getVariablePointer(data->object.cond.datatype1, val1);
    data->object.cond.val2 = getVariablePointer(data->object.cond.datatype2, val2);

    return data;
}
