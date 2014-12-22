#include <stdio.h>
#include <string.h>
#include "nodes.hh"
#include "geometry.hh"
#include "opengl_draw.hh"

#define MAX_STRING_LEN 256

static int count_lines(char *);
static int str_segment_len(char *);


void render_string(struct node *current)
{
    float left, right, bottom, top;
    int num_lines, i, strptr=0, seglen;
    char mystr[MAX_STRING_LEN], tmpstr[MAX_STRING_LEN];
    Geometry geo = GetGeometry(current);

    switch (current->object.string.datatype)
    {
        case FLOAT:
            sprintf(mystr, current->object.string.format, *(float *)current->object.string.value);
            break;
        case INTEGER:
            sprintf(mystr, current->object.string.format, *(int *)current->object.string.value);
            break;
        case STRING:
            sprintf(mystr, current->object.string.format, (char *)current->object.string.value);
            break;
    }

    num_lines = count_lines(mystr);
    for (i=1; i<=num_lines; i++)
    {
        seglen = str_segment_len(&mystr[strptr]);
        strncpy(tmpstr, &mystr[strptr], seglen);
        tmpstr[seglen] = '\0';
        strptr += seglen+2;
        switch (current->object.string.halign)
        {
            case AlignLeft:
                left = 0;
                break;
            case AlignCenter:
                left = - 0.5 * get_string_width(current->object.string.fontID, *(current->object.string.fontSize), current->object.string.forcemono, tmpstr);
                break;
            case AlignRight:
                left = - get_string_width(current->object.string.fontID, *(current->object.string.fontSize), current->object.string.forcemono, tmpstr);
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
            right = left + get_string_width(current->object.string.fontID, *(current->object.string.fontSize), current->object.string.forcemono, tmpstr);
            rectangle_fill((*(current->object.string.bgcolor.R)), (*(current->object.string.bgcolor.G)), (*(current->object.string.bgcolor.B)), (*(current->object.string.bgcolor.A)),
                           left, bottom, left, top, right, top, right, bottom);
        }
        if (*(current->object.string.shadowOffset))
        {
            draw_string(left+(*(current->object.string.shadowOffset)), bottom-(*(current->object.string.shadowOffset)), *(current->object.string.fontSize),
                        0, 0, 0, 1, current->object.string.fontID, current->object.string.forcemono, tmpstr);
        }
        draw_string(left, bottom, *(current->object.string.fontSize),
                    (*(current->object.string.color.R)), (*(current->object.string.color.G)), (*(current->object.string.color.B)), (*(current->object.string.color.A)), current->object.string.fontID, current->object.string.forcemono, tmpstr);

        rotate_end();
        translate_end();
    }
}

static int count_lines(char *str)
{
    int i, count=1;
    int len = strlen(str);

    if (!len) return 0;

    for (i=0; i<(len-1); i++)
    {
        if (str[i] == '\\' && str[i+1] == 'n') count++;
    }

    return count;
}

static int str_segment_len(char *str)
{
    int i;
    int len = strlen(str);

    if (!len) return 0;

    for (i=0; i<(len-1); i++)
    {
        if (str[i] == '\\' && str[i+1] == 'n') return i;
    }

    return len;
}
