#ifndef _NODES_HH_
#define _NODES_HH_

#include "Objects.hh"

#define UNDEFINED_TYPE 0
#define STRING_TYPE    1
#define FLOAT_TYPE     2
#define INTEGER_TYPE   3

union objects
{
    struct PreProcessConstant ppconst;
    struct Default dflt;
    struct Style style;
    struct Constants constants;
    struct Fonts fonts;
    struct ShMem shmem;
    struct Textures textures;
    struct Window win;
    struct Panel panel;
    struct Image image;
    struct PixelStream pixelstream;
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
    unsigned int selected;

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
extern void FreeNode(struct node *);

#endif
