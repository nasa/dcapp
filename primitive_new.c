#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <sys/shm.h>
#include "simio.h"
#include "nodes.h"
#include "string_utils.h"
#include "opengl_draw.h"

extern void window_init(int, int , int, int, int);
extern void *LoadFont(char *, char *);
extern void *LoadShm(key_t);
extern int LoadTexture(char *);
extern void *LoadConstant(int, void *);

extern appdata AppData;

struct node *new_mouseevent(struct node *, struct node **, char *, char *, char *, char *, char *, char *);
struct node *new_keyboardevent(struct node *, struct node **, char *, char *);
struct node *new_bezelevent(struct node *, struct node **, char *);
struct node *new_setvalue(struct node *, struct node **, char *, char *, char *, char *, char *);
struct node *new_isequal(struct node *, struct node **, char *, char *, char *);

static struct node *add_primitive_node(struct node *, struct node **, Type, char *, char *, char *, char *, char *, char *, char *);
static void set_geometry(struct node *, char *, char *, char *, char *, char *, char *, char *);
static void *get_data_pointer(int, char *, void *);
static int get_data_type(char *);

static float fzero = 0;
static int izero = 0;


void new_window(int fullscreen, int OriginX, int OriginY, int SizeX, int SizeY)
{
    NewNode(NULL, &(AppData.window));

    window_init(fullscreen, OriginX, OriginY, SizeX, SizeY);
    graphics_init();
}

void new_panels(char *var)
{
    AppData.window->object.win.active_display = get_data_pointer(INTEGER, var, &izero);
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
    sscanf(colorspec, "%f %f %f", &(data->object.panel.color.R), &(data->object.panel.color.G), &(data->object.panel.color.B));

    return data;
}

struct node *new_container(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *vwidth, char *vheight, char *rotate)
{
    struct node *data = add_primitive_node(parent, list, Container, x, y, width, height, halign, valign, rotate);

    data->object.cont.vwidth = get_data_pointer(FLOAT, vwidth, data->info.w);
    data->object.cont.vheight = get_data_pointer(FLOAT, vheight, data->info.h);

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
    data->object.line.color = StrToColor(color, 1, 1, 1);

    return data;
}

struct node *new_polygon(struct node *parent, struct node **list, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Polygon, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (fillcolor) data->object.poly.fill = 1;
    if (linecolor || linewidth) data->object.poly.outline = 1;

    data->object.poly.FillColor = StrToColor(fillcolor, 1, 1, 1);
    data->object.poly.LineColor = StrToColor(linecolor, 1, 1, 1);
    data->object.poly.linewidth = StrToFloat(linewidth, 1);

    return data;
}

struct node *new_rectangle(struct node *parent, struct node **list, char *x, char *y, char *width, char *height,
                           char *halign, char *valign, char *rotate, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Rectangle, x, y, width, height, halign, valign, rotate);

    if (fillcolor) data->object.rect.fill = 1;
    if (linecolor || linewidth) data->object.rect.outline = 1;

    data->object.rect.FillColor = StrToColor(fillcolor, 1, 1, 1);
    data->object.rect.LineColor = StrToColor(linecolor, 1, 1, 1);
    data->object.rect.linewidth = StrToFloat(linewidth, 1);

    return data;
}

struct node *new_circle(struct node *parent, struct node **list, char *x, char *y, char *halign, char *valign, char *radius,
                        char *segments, char *fillcolor, char *linecolor, char *linewidth)
{
    struct node *data = add_primitive_node(parent, list, Circle, x, y, NULL, NULL, halign, valign, NULL);

    if (fillcolor) data->object.circle.fill = 1;
    if (linecolor || linewidth) data->object.circle.outline = 1;

    data->object.circle.radius = get_data_pointer(FLOAT, radius, &fzero);
    data->object.circle.segments = StrToInt(segments, 80);
    data->object.circle.FillColor = StrToColor(fillcolor, 1, 1, 1);
    data->object.circle.LineColor = StrToColor(linecolor, 1, 1, 1);
    data->object.circle.linewidth = StrToFloat(linewidth, 1);

    return data;
}

struct node *new_string(struct node *parent, struct node **list, char *x, char *y, char *rotate, char *fontsize, char *halign, char *valign,
                        char *color, char *bgcolor, char *shadowoffset, char *font, char *face, char *format, char *forcemono, char *value)
{
    struct node *data = add_primitive_node(parent, list, String, x, y, "0", fontsize, halign, valign, rotate);

    data->object.string.fontSize = StrToFloat(fontsize, 12);
    data->object.string.halign = data->info.halign;
    data->object.string.valign = data->info.valign;

    data->object.string.forcemono = flMonoNone;
    if (forcemono)
    {
        if (!strcmp(forcemono, "Numeric")) data->object.string.forcemono = flMonoNumeric;
        else if (!strcmp(forcemono, "AlphaNumeric")) data->object.string.forcemono = flMonoAlphaNumeric;
        else if (!strcmp(forcemono, "All")) data->object.string.forcemono = flMonoAll;
    }

    data->object.string.color = StrToColor(color, 1, 1, 1);
    if (bgcolor)
    {
        data->object.string.background = 1;
        data->object.string.bgcolor = StrToColor(bgcolor, 0, 0, 0);
    }
    data->object.string.shadowoffset = StrToInt(shadowoffset, 0);
    data->object.string.fontID = LoadFont(font, face);

    data->object.string.value = get_data_pointer(STRING, value, "");
    data->object.string.datatype = get_data_type(value);
    if (data->object.string.datatype == UNDEFINED) data->object.string.datatype = STRING;
    if (format != NULL) data->object.string.format = strdup(format);
    else
    {
        switch (data->object.string.datatype)
        {
            case FLOAT:
                data->object.string.format = strdup("%.1f");
                break;
            case INTEGER:
                data->object.string.format = strdup("%d");
                break;
            case STRING:
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

struct node *new_pixel_stream(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate, char *shm_key, char *filename)
{
    struct node *data = add_primitive_node(parent, list, PixelStream, x, y, width, height, halign, valign, rotate);

    data->object.pixelstream.shm = LoadShm(StrToInt(shm_key, 1234));
    data->object.pixelstream.filename = strdup(filename);
    init_texture(&(data->object.pixelstream.textureID));

    return data;
}

struct node *new_button(struct node *parent, struct node **list, char *x, char *y, char *width, char *height, char *halign, char *valign,
                        char *rotate, char *type, char *switchid, char *switchonval, char *switchoffval, char *indid, char *indonval,
                        char *activevar, char *activeval, char *transitionid, char *key, char *keyascii, char *bezelkey)
{
    int toggle=0, momentary=0;
    struct node *cond, *event, *curlist, **sublist, *list1, *list2, *list3, *list4, *list5, *list6;
    char *offval, *zerostr={"0"}, *onestr={"1"};
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

    data->object.adi.outerradius = get_data_pointer(FLOAT, outerradius, &defouterrad);
    data->object.adi.ballradius = get_data_pointer(FLOAT, ballradius, &defballrad);
    data->object.adi.chevronW = get_data_pointer(FLOAT, chevronwidth, &defchevron);
    data->object.adi.chevronH = get_data_pointer(FLOAT, chevronheight, &defchevron);

    data->object.adi.roll = get_data_pointer(FLOAT, roll, &fzero);
    data->object.adi.pitch = get_data_pointer(FLOAT, pitch, &fzero);
    data->object.adi.yaw = get_data_pointer(FLOAT, yaw, &fzero);

    data->object.adi.rollError = get_data_pointer(FLOAT, roll_error, &fzero);
    data->object.adi.pitchError = get_data_pointer(FLOAT, pitch_error, &fzero);
    data->object.adi.yawError = get_data_pointer(FLOAT, yaw_error, &fzero);

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

struct node *new_setvalue(struct node *parent, struct node **list, char *var, char *optype, char *min, char *max, char *val)
{
    int datatype1 = get_data_type(var);
    int datatype2 = get_data_type(val);
    int mindatatype;
    int maxdatatype;

    if (datatype1 == UNDEFINED) return NULL;

    struct node *data = add_primitive_node(parent, list, SetValue, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (optype == NULL) data->object.modval.optype = Equals;
    else if (!strcmp(optype, "+=")) data->object.modval.optype = PlusEquals;
    else if (!strcmp(optype, "-=")) data->object.modval.optype = MinusEquals;
    else data->object.modval.optype = Equals;

    data->object.modval.datatype1 = datatype1;
    data->object.modval.var = get_data_pointer(datatype1, var, NULL);

    if (datatype2 == UNDEFINED) datatype2 = datatype1;
    data->object.modval.datatype2 = datatype2;
	switch (datatype2)
	{
        case FLOAT:
            data->object.modval.val = get_data_pointer(FLOAT, val, &fzero);
            break;
        case INTEGER:
            data->object.modval.val = get_data_pointer(INTEGER, val, &izero);
            break;
        case STRING:
            data->object.modval.val = get_data_pointer(STRING, val, "");
            break;
	}

    if (min)
    {
        mindatatype = get_data_type(min);
        if (mindatatype == UNDEFINED) mindatatype = datatype1;
        data->object.modval.mindatatype = mindatatype;
        switch (mindatatype)
        {
            case FLOAT:
                data->object.modval.min = get_data_pointer(FLOAT, min, &fzero);
                break;
            case INTEGER:
                data->object.modval.min = get_data_pointer(INTEGER, min, &izero);
                break;
            case STRING:
                data->object.modval.min = get_data_pointer(STRING, min, "");
                break;
        }
    }

    if (max)
    {
        maxdatatype = get_data_type(max);
        if (maxdatatype == UNDEFINED) maxdatatype = datatype1;
        data->object.modval.maxdatatype = maxdatatype;
        switch (maxdatatype)
        {
            case FLOAT:
                data->object.modval.max = get_data_pointer(FLOAT, max, &fzero);
                break;
            case INTEGER:
                data->object.modval.max = get_data_pointer(INTEGER, max, &izero);
                break;
            case STRING:
                data->object.modval.min = get_data_pointer(STRING, max, "");
                break;
        }
    }

    return data;
}

struct node *new_isequal(struct node *parent, struct node **list, char *opspec, char *val1, char *val2)
{
    char *onestr = {"1"};

    if (!val1 && !val2) return NULL;

    struct node *data = add_primitive_node(parent, list, Condition, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (parent) data->info.w = parent->info.w;
    if (parent) data->info.h = parent->info.h;

    if (opspec)
        data->object.cond.opspec = IfEquals;
    else
        data->object.cond.opspec = Simple;

    data->object.cond.datatype1 = get_data_type(val1);
    data->object.cond.datatype2 = get_data_type(val2);
    if (data->object.cond.datatype1 == UNDEFINED && data->object.cond.datatype2 == UNDEFINED)
    {
        data->object.cond.datatype1 = STRING;
        data->object.cond.datatype2 = STRING;
    }
    else if (data->object.cond.datatype1 == UNDEFINED)
        data->object.cond.datatype1 = data->object.cond.datatype2;
    else if (data->object.cond.datatype2 == UNDEFINED)
        data->object.cond.datatype2 = data->object.cond.datatype1;

    if (!val1) val1 = onestr;
    if (!val2) val2 = onestr;

    data->object.cond.val1 = get_data_pointer(data->object.cond.datatype1, val1, NULL);
    data->object.cond.val2 = get_data_pointer(data->object.cond.datatype2, val2, NULL);

    return data;
}

static struct node *add_primitive_node(struct node *parent, struct node **list, Type type, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate)
{
    struct node *data = NewNode(parent, list);
    data->info.type = type;
    set_geometry(data, x, y, width, height, halign, valign, rotate);
    return data;
}

static void set_geometry(struct node *data, char *x, char *y, char *width, char *height, char *halign, char *valign, char *rotate)
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
    if (x) data->info.x = get_data_pointer(FLOAT, x, &fzero);
    else data->info.x = 0;
    if (y) data->info.y = get_data_pointer(FLOAT, y, &fzero);
    else data->info.y = 0;

    data->info.halign = StrToHAlign(halign, AlignLeft);
    data->info.valign = StrToVAlign(valign, AlignBottom);
    data->info.w = get_data_pointer(FLOAT, width, data->info.containerW);
    data->info.h = get_data_pointer(FLOAT, height, data->info.containerH);
    data->info.rotate = get_data_pointer(FLOAT, rotate, &fzero);
}

static void *get_data_pointer(int type, char *valstr, void *defval)
{
    float fval;
    int ival;
    char *sval, *inptr;

    if (valstr)
    {
        if (strlen(valstr) > 1)
        {
            if (valstr[0] == '@') return get_pointer(&valstr[1]);
        }
    }

    switch (type)
    {
        case FLOAT:
            if (defval)
                fval = StrToFloat(valstr, *(float *)defval);
            else
                fval = StrToFloat(valstr, 0);
            return LoadConstant(type, &fval);
        case INTEGER:
            if (defval)
                ival = StrToInt(valstr, *(int *)defval);
            else
                ival = StrToInt(valstr, 0);
            return LoadConstant(type, &ival);
        case STRING:
            inptr = valstr;
            if (valstr)
            {
                if (valstr[0] == '\\') inptr = &valstr[1];
            }
            if (defval)
                sval = StrToStr(inptr, (char *)defval);
            else
                sval = StrToStr(inptr, "");
            return LoadConstant(type, sval);
        default:
            return 0;
    }
}

static int get_data_type(char *valstr)
{
    if (valstr)
    {
        if (strlen(valstr) > 1)
        {
            if (valstr[0] == '@') return get_datatype(&valstr[1]);
        }
    }
    return UNDEFINED;
}
