#include "opengl_draw.hh"
#include "geometry.hh"
#include "alignment.hh"
#include "string.hh"

dcString::dcString(float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrot, bool bkgd, Kolor *incolor, Kolor *inbgcolor, std::vector<VarString *> invstr, std::vector<std::string> infil, flFont *infont, float *insize, float *inshadow, flMonoOption inforce)
{
    x = inx;
    y = iny;
    w = inw;
    h = inh;
    containerw = incw; // TODO: these should come from the parent
    containerh = inch; // TODO: these should come from the parent
    halign = inhal;
    valign = inval;
    rotate = inrot;
    background = bkgd;
    color.R = incolor->R;
    color.G = incolor->G;
    color.B = incolor->B;
    color.A = incolor->A;
    bgcolor.R = inbgcolor->R;
    bgcolor.G = inbgcolor->G;
    bgcolor.B = inbgcolor->B;
    bgcolor.A = inbgcolor->A;
    vstring = invstr;
    filler = infil;
    fontID = infont;
    fontSize = insize;
    shadowOffset = inshadow;
    forcemono = inforce;
}

static unsigned count_lines(std::string instr)
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

void dcString::draw(void)
{
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
