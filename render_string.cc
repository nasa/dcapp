#include <cstdio>
#include <cstring>
#include <string>
#include "nodes.hh"
#include "geometry.hh"
#include "opengl_draw.hh"

static unsigned count_lines(std::string);

void render_string(struct node *current)
{
    float left, right, bottom, top;
    unsigned num_lines, i, strptr=0, seglen;
    Geometry geo = GetGeometry(current);

    std::string mystring;
    for (i=0; i<current->vstring.size(); i++) mystring += current->filler[i] + current->vstring[i]->get();
    mystring += current->filler[current->vstring.size()];

    num_lines = count_lines(mystring);
    for (i=1; i<=num_lines; i++)
    {
        seglen = mystring.find("\\n", strptr) - strptr;
        std::string tmpstr = mystring.substr(strptr, seglen);
        strptr += seglen+2;

        switch (current->object.string.halign)
        {
            case AlignLeft:
                left = 0;
                break;
            case AlignCenter:
                left = - 0.5 * get_string_width(current->object.string.fontID, *(current->object.string.fontSize), current->object.string.forcemono, tmpstr.c_str());
                break;
            case AlignRight:
                left = - get_string_width(current->object.string.fontID, *(current->object.string.fontSize), current->object.string.forcemono, tmpstr.c_str());
                break;
        }

        switch (current->object.string.valign)
        {
            case AlignBottom:
                bottom = *(current->object.string.fontSize) * (float)(num_lines - i);
                break;
            case AlignMiddle:
                bottom = *(current->object.string.fontSize) * (((float)num_lines/2) - (float)i);
                break;
            case AlignTop:
                bottom = -(*(current->object.string.fontSize)) * (float)i;
                break;
        }

        translate_start(geo.refx, geo.refy);
        rotate_start(*(current->info.rotate));

        if (current->object.string.background)
        {
            top = bottom + *(current->object.string.fontSize);
            right = left + get_string_width(current->object.string.fontID, *(current->object.string.fontSize), current->object.string.forcemono, tmpstr.c_str());
            rectangle_fill((*(current->object.string.bgcolor.R)), (*(current->object.string.bgcolor.G)), (*(current->object.string.bgcolor.B)), (*(current->object.string.bgcolor.A)),
                           left, bottom, left, top, right, top, right, bottom);
        }
        if (*(current->object.string.shadowOffset))
        {
            draw_string(left+(*(current->object.string.shadowOffset)), bottom-(*(current->object.string.shadowOffset)), *(current->object.string.fontSize),
                        0, 0, 0, 1, current->object.string.fontID, current->object.string.forcemono, tmpstr.c_str());
        }
        draw_string(left, bottom, *(current->object.string.fontSize), (*(current->object.string.color.R)), (*(current->object.string.color.G)), (*(current->object.string.color.B)), (*(current->object.string.color.A)),
                    current->object.string.fontID, current->object.string.forcemono, tmpstr.c_str());

        rotate_end();
        translate_end();
    }
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
