#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "geometry.hh"
#include "alignment.hh"
#include "string.hh"

extern float *getFloatPointer(const char *); // TODO: put in header file
extern char *getStringPointer(const char *); // TODO: put in header file
extern int get_data_type(const char *); // TODO: put in header file

dcString::dcString(dcParent *myparent)
:
x(0x0),
y(0x0),
halign(AlignLeft),
valign(AlignBottom),
fontID(0x0)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
    w = containerw;
    h = containerh;
    rotate = dcLoadConstant(0.0f);
    background = false;
    color.R = dcLoadConstant(1.0f);
    color.G = dcLoadConstant(1.0f);
    color.B = dcLoadConstant(1.0f);
    color.A = dcLoadConstant(1.0f);
    fontSize = dcLoadConstant(12.0f);
    shadowOffset = dcLoadConstant(0.0f);
    forcemono = flMonoNone;
}

void dcString::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getFloatPointer(inx);
    if (iny) y = getFloatPointer(iny);
}

void dcString::setSize(const char *inw, const char *inh)
{
    if (inw) w = getFloatPointer(inw);
    if (inh) h = getFloatPointer(inh);
}

void dcString::setRotation(const char *inr)
{
    if (inr) rotate = getFloatPointer(inr);
}

void dcString::setAlignment(const char *inhal, const char *inval)
{
    if (inhal)
    {
        if (!strcasecmp(inhal, "Left")) halign = AlignLeft;
        else if (!strcasecmp(inhal, "Center")) halign = AlignCenter;
        else if (!strcasecmp(inhal, "Right")) halign = AlignRight;
    }
    if (inval)
    {
        if (!strcasecmp(inval, "Bottom")) valign = AlignBottom;
        else if (!strcasecmp(inval, "Middle")) valign = AlignMiddle;
        else if (!strcasecmp(inval, "Top")) valign = AlignTop;
    }
}

void dcString::setColor(const char *cspec)
{
    if (cspec) color = StrToColor(cspec, 1, 1, 1, 1);
}

void dcString::setBackgroundColor(const char *cspec)
{
    if (cspec)
    {
        background = true;
        bgcolor = StrToColor(cspec, 0, 0, 0, 1);
    }
}

void dcString::setFont(const char *font, const char *face, const char *size, const char *mono)
{
    fontID = dcLoadFont(font, face);
    if (size) fontSize = getFloatPointer(size);
    if (mono)
    {
        if (!strcmp(mono, "Numeric")) forcemono = flMonoNumeric;
        else if (!strcmp(mono, "AlphaNumeric")) forcemono = flMonoAlphaNumeric;
        else if (!strcmp(mono, "All")) forcemono = flMonoAll;
    }
}

void dcString::setShadowOffset(const char *inval)
{
    if (inval) shadowOffset = getFloatPointer(inval);
}

void dcString::setString(std::string mystr)
{
    size_t vstart, vlen, curpos = 0;

    do
    {
        vstart = mystr.find('@', curpos);
        filler.push_back(mystr.substr(curpos, vstart-curpos));
        if (vstart == std::string::npos) return;
        vlen = parse_var(mystr.substr(vstart, std::string::npos));
        if (vlen == std::string::npos)
        {
            filler.push_back("");
            return;
        }
        curpos = vstart + vlen;
    } while (curpos < mystr.size());

    filler.push_back("");
}

void dcString::draw(void)
{
    if (!fontID) return;

    float left, right, bottom, top;
    unsigned num_lines, i, strptr=0, seglen;
    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);

    std::string mystring;
    for (i = 0; i < vstring.size(); i++) mystring += filler[i] + vstring[i]->get();
    mystring += filler[vstring.size()];

    num_lines = count_lines(mystring);

    for (i = 1; i <= num_lines; i++)
    {
        seglen = mystring.find("\\n", strptr) - strptr;
        std::string tmpstr = mystring.substr(strptr, seglen);
        strptr += seglen+2;

        switch (halign)
        {
            case AlignLeft:
                left = 0;
                break;
            case AlignCenter:
                left = - 0.5 * get_string_width(fontID, *fontSize, forcemono, tmpstr.c_str());
                break;
            case AlignRight:
                left = - get_string_width(fontID, *fontSize, forcemono, tmpstr.c_str());
                break;
        }

        switch (valign)
        {
            case AlignBottom:
                bottom = (*fontSize) * (float)(num_lines - i);
                break;
            case AlignMiddle:
                bottom = (*fontSize) * (((float)num_lines/2) - (float)i);
                break;
            case AlignTop:
                bottom = -(*fontSize) * (float)i;
                break;
        }

        translate_start(geo.refx, geo.refy);
        rotate_start(*rotate);

        if (background)
        {
            top = bottom + (*fontSize);
            right = left + get_string_width(fontID, *fontSize, forcemono, tmpstr.c_str());
            rectangle_fill(*(bgcolor.R), *(bgcolor.G), *(bgcolor.B), *(bgcolor.A), left, bottom, left, top, right, top, right, bottom);
        }
        if (*shadowOffset)
        {
            draw_string(left+(*shadowOffset), bottom-(*shadowOffset), *fontSize, 0, 0, 0, 1, fontID, forcemono, tmpstr.c_str());
        }
        draw_string(left, bottom, *fontSize, *(color.R), *(color.G), *(color.B), *(color.A), fontID, forcemono, tmpstr.c_str());

        rotate_end();
        translate_end();
    }
}

// TODO: The parsing in this file should be simplified and/or combined with string parsing in xml_stringsub.cc
size_t dcString::parse_var(std::string mystr)
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
        vstring.push_back(new VarString(get_data_type(varstr.c_str()), getStringPointer(varstr.c_str()), mystr.substr(fmt_start+1, fmt_end-fmt_start-1).c_str()));
    else
        vstring.push_back(new VarString(get_data_type(varstr.c_str()), getStringPointer(varstr.c_str()), 0x0));

    if (braced) return mystr.find('}') + 1;
    else if (fmt_end != std::string::npos) return fmt_end + 1;
    else return mystr.find(' ');
}

unsigned dcString::count_lines(std::string instr)
{
    if (!(instr.size())) return 0;

    unsigned count = 0;
    size_t next, pos = 0;

    do
    {
        next = instr.find("\\n", pos);
        count++;
        pos = next+2;
    } while (next != std::string::npos);

    return count;
}
