#ifndef _NODES_HH_
#define _NODES_HH_

#include <vector>
#include "Objects.hh"

#define UNDEFINED_TYPE 0
#define STRING_TYPE    1
#define FLOAT_TYPE     2
#define INTEGER_TYPE   3

class VarString
{
    public:
        VarString(int a, void *b, const char *c)
        {
            if (a == UNDEFINED_TYPE) datatype = STRING_TYPE;
            else datatype = a;
            value = b;
            if (c) format = c;
            else
            {
                switch (datatype)
                {
                    case FLOAT_TYPE:   format = "%g"; break;
                    case INTEGER_TYPE: format = "%d";   break;
                    case STRING_TYPE:  format = "%s";   break;
                }
            }
        };
        std::string get(void)
        {
            char *tmp_str = 0x0;
            switch (datatype)
            {
                case FLOAT_TYPE:   asprintf(&tmp_str, format.c_str(), *(float *)(value)); break;
                case INTEGER_TYPE: asprintf(&tmp_str, format.c_str(), *(int *)(value));   break;
                case STRING_TYPE:  asprintf(&tmp_str, format.c_str(), (char *)value);     break;
            }
            std::string ret_str = tmp_str;
            free(tmp_str);
            return ret_str;
        };
    private:
        int datatype;
        void *value;
        std::string format;
};

union objects
{
    struct Window win;
    struct Panel panel;
    struct Image image;
    struct PixelStreamView pixelstreamview;
    struct Container cont;
    struct Animate anim;
    struct Condition cond;
    struct Line line;
    struct Polygon poly;
    struct Rect rect;
    struct Circle circle;
    struct String string;
    struct ADI adi;
    struct MouseEvent me;
    struct KeyboardEvent ke;
    struct BezelEvent be;
    struct ModifyValue modval;
};

// These items are common to many primitives and are stored in a standard structure for easy access
struct info
{
    Type type;
    int halign;
    int valign;
    bool selected;

    float *x;
    float *y;
    float *w;
    float *h;
    float *containerW;
    float *containerH;
    float *rotate;
};

struct node
{
    struct info info;
    union objects object;
    // The two items below, and the above VarString class, should be defined in the String structure,
    // but the String structure is a member of the objects union, so it can't hold a variable size class
    std::vector<VarString *>vstring;
    std::vector<std::string>filler;
    struct node *p_prev;
    struct node *p_prev_list;
    struct node *p_next;
    struct node *p_next_list;
    struct node *p_head;
    struct node *p_tail;
    struct node *p_current;
    struct node *p_parent;
};

/* Function Protytypes */
extern struct node *NewList(struct node *);
extern struct node *NewNode(struct node *, struct node **);

#endif
