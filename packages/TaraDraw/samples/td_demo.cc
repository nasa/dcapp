// A somewhat-complex demo that demonstrates the usage of most of TaraDraw's
// core capabilities

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <climits>
#include <math.h>
#include <TaraDraw/TaraDraw.hh>
#include <OpenGL/gl.h>

#define NUMBUTTONS 7
#define NUMWINDOWS (NUMBUTTONS+1)

enum { MainWindow,ColorWindow,LineWindow,ShapeWindow,TextWindow,EventWindow,ImageWindow,GlWindow };

static struct window
{
    tdWindow id;
    int alive;
    float width;
    float height;
    char title[32];
    tdGLContext *gl_context;
} Window[NUMWINDOWS];

static struct button
{
    float left;
    float right;
    float bottom;
    float top;
    char str[12];
    struct window *win;
} Button[NUMBUTTONS];

static struct image {
    tdImage *image;
    tdSize size;
} Image;

void app_run(void);
void app_term(void);
void mouse_click(ButtonEvent);
void key_click(KeyboardEvent);
void win_config(ConfigureEvent);
void win_close(WinCloseEvent);

static void set_window_attributes(struct window *, float, float, const char *);
static void set_button_attributes(struct button *, const char *, struct window *);
static void create_window(struct window *);
static void draw_main_window(struct window *);
static void draw_color_window(struct window *);
static void draw_line_window(struct window *);
static void draw_shape_window(struct window *);
static void draw_text_window(struct window *);
static void draw_event_window(struct window *);
static void draw_image_window(struct window *);
static void draw_gl_window(struct window *);

static char EventStr[80] = {"Click mouse, press key, or reconfigure window"};

int main(int argc, const char *argv[])
{
    char *myarg=strdup(argv[0]);
    char *relpath;
    char *fullpath = (char *)calloc(PATH_MAX, sizeof(char));

    asprintf(&relpath, "%s/../nasalogo.bmp", dirname(myarg));
    realpath(relpath, fullpath);

    bzero(Window, sizeof(Window));

    tdInitialize("MainMenu");

    Image.image = tdLoadImage(fullpath, &Image.size);
    if (Image.image == 0)
    {
        Image.size.width = 400;
        Image.size.height = 100;
    }

    free(fullpath);
    free(relpath);
    free(myarg);

    // Set default attributes for each window
    set_window_attributes(&Window[MainWindow], 600, 100, "Main Window");
    set_window_attributes(&Window[ColorWindow], 280, 80, "Color Window");
    set_window_attributes(&Window[LineWindow], 300, 300, "Line Window");
    set_window_attributes(&Window[ShapeWindow], 210, 75, "Shape Window");
    set_window_attributes(&Window[TextWindow], 300, 150, "Text Window");
    set_window_attributes(&Window[EventWindow], 300, 80, "Event Window");
    set_window_attributes(&Window[ImageWindow], Image.size.width, Image.size.height, "Image Window");
    set_window_attributes(&Window[GlWindow], 200, 200, "OpenGL Window");

    // Set attributes for main window buttons
    set_button_attributes(&Button[ColorWindow-1], "Colors", &Window[ColorWindow]);
    set_button_attributes(&Button[LineWindow-1], "Lines", &Window[LineWindow]);
    set_button_attributes(&Button[ShapeWindow-1], "Shapes", &Window[ShapeWindow]);
    set_button_attributes(&Button[TextWindow-1], "Text", &Window[TextWindow]);
    set_button_attributes(&Button[EventWindow-1], "Events", &Window[EventWindow]);
    set_button_attributes(&Button[ImageWindow-1], "Images", &Window[ImageWindow]);
    set_button_attributes(&Button[GlWindow-1], "OpenGL", &Window[GlWindow]);

    tdRegisterColor(0, 1.0, 1.0, 1.0); // white
    tdRegisterColor(1, 0.0, 0.0, 0.0); // black
    tdRegisterColor(2, 1.0, 0.0, 0.0); // red
    tdRegisterColor(3, 0.0, 1.0, 0.0); // green
    tdRegisterColor(4, 0.0, 0.0, 1.0); // blue
    tdRegisterColor(5, 1.0, 0.0, 1.0); // purple
    tdRegisterColor(6, 1.0, 1.0, 0.5); // yellow
    tdRegisterColor(7, 0.0, 0.5, 0.5); // dark green
    tdRegisterColor(8, 0.8, 0.8, 0.8); // light gray
    tdRegisterColor(9, 0.5, 0.5, 0.5); // medium gray

    create_window(&Window[MainWindow]);
    tdMainLoop(app_run, app_term);
    return 0;
}

void app_run(void)
{
    tdProcessEvents(mouse_click, key_click, win_config, win_close);
    if (Window[MainWindow].alive && tdNeedsRedraw(Window[MainWindow].id))
        draw_main_window(&Window[MainWindow]);
    if (Window[ColorWindow].alive && tdNeedsRedraw(Window[ColorWindow].id))
        draw_color_window(&Window[ColorWindow]);
    if (Window[LineWindow].alive && tdNeedsRedraw(Window[LineWindow].id))
        draw_line_window(&Window[LineWindow]);
    if (Window[ShapeWindow].alive && tdNeedsRedraw(Window[ShapeWindow].id))
        draw_shape_window(&Window[ShapeWindow]);
    if (Window[TextWindow].alive && tdNeedsRedraw(Window[TextWindow].id))
        draw_text_window(&Window[TextWindow]);
    if (Window[EventWindow].alive && tdNeedsRedraw(Window[EventWindow].id))
        draw_event_window(&Window[EventWindow]);
    if (Window[ImageWindow].alive && tdNeedsRedraw(Window[ImageWindow].id))
        draw_image_window(&Window[ImageWindow]);
    if (Window[GlWindow].alive && tdNeedsRedraw(Window[GlWindow].id))
        draw_gl_window(&Window[GlWindow]);
}

static void set_window_attributes(struct window *win, float width, float height, const char *title)
{
    win->width = width;
    win->height = height;
    strcpy(win->title, title);
}

static void set_button_attributes(struct button *but, const char *string, struct window *win)
{
    strcpy(but->str, string);
    but->win = win;
} 

static void create_window(struct window *win)
{
    tdRegion usable;
    static float xpos, ypos;
    static int firstpass = 1;

    tdGetScreenData(NULL, &usable);
    if (firstpass)
    {
        xpos = usable.left;
        ypos = usable.top;
        firstpass = 0;
    }
    else
    {
        xpos += 20;
        if (xpos + win->width > usable.right) xpos = usable.left;
        ypos -= 20;
        if (ypos - win->height < usable.bottom) ypos = usable.top;
    }

    win->alive = 1;
    win->id = tdOpenWindow(win->title, xpos, ypos, win->width, win->height, tdAlignLeft | tdAlignTop);
    if (win->id == Window[MainWindow].id)
        draw_main_window(win);
    else if (win->id == Window[ColorWindow].id)
        draw_color_window(win);
    else if (win->id == Window[LineWindow].id)
        draw_line_window(win);
    else if (win->id == Window[ShapeWindow].id)
        draw_shape_window(win);
    else if (win->id == Window[TextWindow].id)
        draw_text_window(win);
    else if (win->id == Window[EventWindow].id)
        draw_event_window(win);
    else if (win->id == Window[ImageWindow].id)
        draw_image_window(win);
    else if (win->id == Window[GlWindow].id)
    {
        Window[GlWindow].gl_context = tdGLCreateContext(win->id);
        draw_gl_window(win);
    }
}

void mouse_click(ButtonEvent btn)
{
    int i, j;

    for (i=0; i<NUMWINDOWS; i++)
    {
        if (btn.window == Window[i].id)
        {
            if (i == MainWindow && btn.state == tdPressed)
            {
                for (j=0; j<NUMBUTTONS; j++)
                {
                    if (btn.pos.x > Button[j].left && btn.pos.x < Button[j].right &&
                        btn.pos.y > Button[j].bottom && btn.pos.y < Button[j].top)
                    {
                        if (!Button[j].win->alive) create_window(Button[j].win);
                    }
                }
            }
            else if (Window[i].alive && i == EventWindow)
            {
                if (btn.state == tdPressed)
                    sprintf(EventStr, "Mouse pressed at %g, %g", btn.pos.x, btn.pos.y);
                else
                    sprintf(EventStr, "Mouse released at %g, %g", btn.pos.x, btn.pos.y);
                draw_event_window(&Window[EventWindow]);
            }
        }
    }
}

void key_click(KeyboardEvent kbd)
{
    int i;

    if (kbd.key == 27 && kbd.state == tdPressed)  // "esc" key
    {
        for (i=0; i<NUMWINDOWS; i++)
        {
            if (Window[i].alive && kbd.window == Window[i].id)
            {
                if (i == MainWindow) app_term();
                else
                {
                    tdSetActiveWindow(kbd.window);
                    tdCloseWindow();
                    Window[i].alive = 0;
                    tdSetActiveWindow(Window[MainWindow].id);
                }
            }
        }
    }
    else if (Window[EventWindow].alive && kbd.window == Window[EventWindow].id)
    {
        if (kbd.state == tdPressed)
            if (kbd.specialkey)
                sprintf(EventStr, "special key (code %d) pressed at %g, %g", kbd.specialkey, kbd.pos.x, kbd.pos.y);
            else
                sprintf(EventStr, "\'%c\' key (ASCII %d) pressed at %g, %g", kbd.key, kbd.key, kbd.pos.x, kbd.pos.y);
        else
            if (kbd.specialkey)
                sprintf(EventStr, "special key (code %d) released at %g, %g", kbd.specialkey, kbd.pos.x, kbd.pos.y);
            else
                sprintf(EventStr, "\'%c\' key (ASCII %d) released at %g, %g", kbd.key, kbd.key, kbd.pos.x, kbd.pos.y);
        draw_event_window(&Window[EventWindow]);
    }
}

void win_config(ConfigureEvent cfg)
{
    int i;

    for (i=0; i<NUMWINDOWS; i++)
    {
        if (Window[i].alive && cfg.window == Window[i].id)
        {
            Window[i].width = cfg.size.width;
            Window[i].height = cfg.size.height;
            if (i == EventWindow) sprintf(EventStr, "Window geometry: %gx%g", cfg.size.width, cfg.size.height);
            if (i == GlWindow)
            {
                tdGLSetContext(Window[i].gl_context);
                tdGLReshapeContext(0, 0, 0, cfg.size.width, cfg.size.height);
            }
        }
    }
}

void win_close(WinCloseEvent wcl)
{
    int i;
    for (i=0; i<NUMWINDOWS; i++)
    {
        if (Window[i].alive && wcl.window == Window[i].id)
        {
            if (i == MainWindow) app_term();
            else
            {
                if (i == GlWindow)
                {
                    tdGLSetContext(Window[i].gl_context);
                    tdGLDestroyContext();
                }
                tdSetActiveWindow(wcl.window);
                tdCloseWindow();
                Window[i].alive = 0;
                tdSetActiveWindow(Window[MainWindow].id);
            }
        }
    }
}

void app_term(void)
{
    tdTerminate();
    exit(0);
}

static void draw_main_window(struct window *win)
{
    int i;
    float btnbottom, btntop, btnwidth, btnheight, btncenter, btnmiddle;
    char strBuf[256];

    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();

    tdSetColor(1);
    tdSetLineWidth(1);
    tdSetLineCap(tdButtLineCapStyle);
    tdSetFont("Helvetica", 12);
    sprintf(strBuf, "Window %dx%d pixels", (int)win->width, (int)win->height);
    tdDrawString(strBuf, win->width-2, win->height-2, 0.0, tdAlignRight | tdAlignTop);
    tdDrawString("<esc> closes a window, <esc> on main window quits the demo",
                 2, 2, 0.0, tdAlignLeft | tdAlignBottom);

    // Internal frame
    tdLineStart(20, 20);
    tdLineAppend(win->width-20, 20);
    tdLineAppend(win->width-20, win->height-20);
    tdLineAppend(20, win->height-20);
    tdLineAppend(20, 20);
    
    // Buttons
    btnwidth = (win->width - 40 - ((NUMBUTTONS+1)*10)) / NUMBUTTONS;
    btnbottom = 30;
    btntop = win->height - 30;
    btnheight = btntop - btnbottom;
    btnmiddle = (btnbottom + btntop) / 2;

    tdSetFont("Helvetica", 14);
    for (i=0; i<NUMBUTTONS; i++)
    {
        Button[i].left = 30+(i*(btnwidth+10));
        Button[i].right = Button[i].left + btnwidth;
        Button[i].bottom = btnbottom;
        Button[i].top = btntop;
        btncenter = (Button[i].left + Button[i].right) / 2;

        tdSetColor(1);
        tdDrawFilledRect(Button[i].left, btnbottom, tdAlignLeft | tdAlignBottom, btnwidth, btnheight);
        tdSetColor(8);
        tdDrawFilledRect(Button[i].left+2, btnbottom+2, tdAlignLeft | tdAlignBottom, btnwidth-4, btnheight-4);
        tdSetColor(1);
        tdDrawString(Button[i].str, btncenter, btnmiddle, 0, tdAlignCenter | tdAlignMiddle);
    }

    tdRenderGraphics();
}

static void draw_color_window(struct window *win)
{
    int i;
    char strBuf[256];
    float unitx, unity, boxtop, boxbottom, boxheight, boxmiddle;
    
    unitx = (float)win->width / 25;
    unity = (float)(win->height - 12) / 5;
    boxtop = (float)(win->height - 12) - (2 * unity);
    boxbottom = unity;
    boxheight = boxtop - boxbottom;
    boxmiddle = boxbottom + (boxheight / 2);

    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();

    tdSetColor(1);
    tdSetFont("Helvetica", 12);
    tdSetLineWidth(1);
    tdDrawString("Custom Colormap (8 out of 256)", win->width/2, boxtop+unity, 0.0, tdAlignCenter | tdAlignBottom);

    // Display the colormap, but first create a background for the white box...
    tdSetColor(8);
    tdDrawFilledRect(unitx-2, boxmiddle, tdAlignLeft | tdAlignMiddle, (unitx*2)+4, (unity*2)+4);

    for (i=0; i<8; i++)
    {
        tdSetColor(i);
        tdDrawFilledRect(unitx+(unitx*i*3), boxmiddle, tdAlignLeft | tdAlignMiddle, unitx*2, boxheight);
        // Print the color index
        if (i==1 || i == 4 || i == 7)
            tdSetColor(0);
        else
            tdSetColor(1);
        sprintf(strBuf, "%d", i);
        tdDrawString(strBuf, (unitx*2)+(unitx*i*3), boxmiddle, 0.0, tdAlignCenter | tdAlignMiddle);
    }
    
    tdRenderGraphics();
}

static void draw_line_window(struct window *win)
{
    int i;
    char strBuf[256];
    float unity, lright, sleft;
    
    unity = (float)win->height / 14;

    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();

    tdSetColor(1);
    tdSetFont("Helvetica", 12);
    tdDrawString("Line Widths and Cap Styles", win->width/2, (13*unity), 0.0, tdAlignCenter | tdAlignMiddle);

    // Line widths
    tdSetColor(1);
    lright = (float)win->width - 90;
    sleft = lright + 10;

    for (i=0; i<6; i++)
    {
        tdSetLineWidth((float)i+.5);
        tdLineStart(10, unity*(6+i));
        tdLineAppend(lright, unity*(6+i));
        sprintf(strBuf, "linewidth %3.1f", (float)i+.5);
        tdDrawString(strBuf, sleft, unity*(6+i), 0.0, tdAlignLeft | tdAlignMiddle);
    }

    // Linecap styles
    lright = (float)win->width - 149.5;
    sleft = lright + 15.5;

    tdSetLineWidth(11.0);
    tdSetColor(1);
    tdSetLineCap(tdButtLineCapStyle);
    tdLineStart(15.5, unity*4);
    tdLineAppend(lright, unity*4);
    tdDrawString("tdButtLineCapStyle", sleft, unity*4, 0.0, tdAlignLeft | tdAlignMiddle);
    tdSetLineWidth(1.0);
    tdSetColor(6);
    tdLineStart(15.5, unity*4);
    tdLineAppend(lright, unity*4);

    tdSetLineWidth(11.0);
    tdSetColor(1);
    tdSetLineCap(tdRoundLineCapStyle);
    tdLineStart(15.5, unity*3);
    tdLineAppend(lright, unity*3);
    tdDrawString("tdRoundLineCapStyle", sleft, unity*3, 0.0, tdAlignLeft | tdAlignMiddle);
    tdSetLineWidth(1.0);
    tdSetColor(6);
    tdLineStart(15.5, unity*3);
    tdLineAppend(lright, unity*3);

    tdSetLineWidth(11.0);
    tdSetColor(1);
    tdSetLineCap(tdSquareLineCapStyle);
    tdLineStart(15.5, unity*2);
    tdLineAppend(lright, unity*2);
    tdDrawString("tdSquareLineCapStyle", sleft, unity*2, 0.0, tdAlignLeft | tdAlignMiddle);
    tdSetLineWidth(1.0);
    tdSetColor(6);
    tdLineStart(15.5, unity*2);
    tdLineAppend(lright, unity*2);

    tdRenderGraphics();
}

static void draw_shape_window(struct window *win)
{
    int i;
    float xpnt[32], ypnt[32];
    float unitx, unity;
    double radians;
    double pi = 4.0*atan(1.0);
    
    unitx = (float)win->width / 7;
    unity = (float)win->height / 5;

    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();

    tdSetColor(1);
    tdSetFont("Helvetica", 12);
    tdSetLineWidth(1);
    tdDrawString("Shapes", win->width/2, unity*4, 0.0, tdAlignCenter | tdAlignBottom);

    // Draw rectangle
    tdSetColor(2);
    tdDrawFilledRect(unitx, unity, tdAlignLeft | tdAlignBottom, unitx, unity*2);

    // Draw triangle
    tdSetColor(3);
    xpnt[0] = unitx*3;
    ypnt[0] = unity;
    xpnt[1] = unitx*4;
    ypnt[1] = unity;
    xpnt[2] = unitx*3.5;
    ypnt[2] = unity*3;
    tdDrawFilledPoly(xpnt, ypnt, 3);
    
    // Draw outline around triangle
    tdSetColor(1);
    tdLineStart(xpnt[0], ypnt[0]);
    tdLineAppend(xpnt[1], ypnt[1]);
    tdLineAppend(xpnt[2], ypnt[2]);
    tdLineAppend(xpnt[0], ypnt[0]);

    // Draw circle
    for (i=0; i<32; i++)
    {
        radians=(double)i*pi/16.0;
        xpnt[i] = (unitx*5.5)+((unitx/2)*cos(radians));
        ypnt[i] = (unity*2)+(unity*sin(radians));
    }
    tdSetColor(4);
    tdDrawFilledPoly(xpnt, ypnt, 32);
    
    tdRenderGraphics();
}

static void draw_text_window(struct window *win)
{
    float x, y;
    tdSize strsize;
    
    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();

    y = win->height - 10;
    tdSetColor(1);
    tdSetFont("Times Italic", 20);
    tdDrawString("Times Italic 20", 10, y, 0.0, tdAlignLeft | tdAlignTop);
    strsize = tdGetStringBounds("Times Italic 20");
    y -= (strsize.height + 4);
    tdSetColor(2);
    tdSetFont("Times", 17);
    tdDrawString("Times 17", 10, y, 0.0, tdAlignLeft | tdAlignTop);
    strsize = tdGetStringBounds("Times 17");
    y -= (strsize.height + 4);
    tdSetColor(4);
    tdSetFont("Courier", 14);
    tdDrawString("Courier 14", 10, y, 0.0, tdAlignLeft | tdAlignTop);
    strsize = tdGetStringBounds("Courier 14");
    y -= (strsize.height + 4);
    tdSetColor(5);
    tdSetFont("Helvetica", 11);
    tdDrawString("Helvetica 11", 11, y, 0.0, tdAlignLeft | tdAlignTop);

    x = win->width-62;
    y = win->height-10;
    tdSetColor(2);
    tdSetLineWidth(0.5);
    tdLineStart(x, y);
    tdLineAppend(x, y-60);

    tdSetColor(1);
    tdSetFont("Lucida Sans", 10);
    tdDrawString("left aligned", x, y-10, 0.0, tdAlignLeft | tdAlignMiddle);
    tdDrawString("centered", x, y-30, 0.0, tdAlignCenter | tdAlignMiddle);
    tdDrawString("right aligned", x, y-50, 0.0, tdAlignRight | tdAlignMiddle);

    x = win->width-44;
    y = 48;
    tdSetColor(2);
    tdLineStart(x+5, y);
    tdLineAppend(x-5, y);
    tdLineStart(x, y+5);
    tdLineAppend(x, y-5);

    tdSetColor(1);
    tdSetFont("Times", 14);
    tdDrawString("-rotate", x, y, 90.0, tdAlignLeft | tdAlignMiddle);
    tdDrawString("-rotate", x, y, 45.0, tdAlignLeft | tdAlignMiddle);
    tdDrawString("-rotate", x, y, -30.0, tdAlignLeft | tdAlignMiddle);
    tdDrawString("-rotate", x, y, -60.0, tdAlignLeft | tdAlignMiddle);
    tdDrawString("-rotate", x, y, -90.0, tdAlignLeft | tdAlignMiddle);

    tdSetColor(2);
    tdSetLineWidth(0.5);
    tdLineStart(10, 24);
    tdLineAppend(210, 24);
    tdSetColor(1);
    tdSetFont("Times Italic", 14);
    tdDrawString("Top", 20, 24, 0.0, tdAlignLeft | tdAlignTop);
    tdDrawString("Bottom", 50, 24, 0.0, tdAlignLeft | tdAlignBottom);
    tdDrawString("Middle", 100, 24, 0.0, tdAlignLeft | tdAlignMiddle);
    tdDrawString("Baseline", 150, 24, 0.0, tdAlignLeft | tdAlignBaseline);
    
    tdRenderGraphics();
}

static void draw_event_window(struct window *win)
{
    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();
    
    tdSetColor(1);
    tdSetFont("Helvetica", 12);
    tdDrawString(EventStr, win->width/2, win->height/2, 0.0, tdAlignCenter | tdAlignMiddle);

    tdRenderGraphics();
}

static void draw_image_window(struct window *win)
{
    tdSetActiveWindow(win->id);
    tdSetBackgroundColor(0);
    tdClearWindow();
    if (Image.image)
        tdDrawImage(Image.image, 0, 0, tdAlignLeft | tdAlignBottom, win->width/Image.size.width, win->height/Image.size.height, 0);
    else
    {
        tdSetColor(1);
        tdSetFont("Helvetica", 12);
        tdDrawString("ERROR: unable to locate nasalogo.bmp image", win->width/2, win->height/2, 0.0, tdAlignCenter | tdAlignMiddle);
    }
    tdRenderGraphics();
}

static void draw_gl_window(struct window *win)
{
    tdGLSetContext(win->gl_context);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, win->width, win->height);
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(0, 0, 1, 1);

    glBegin(GL_POLYGON);
        glVertex3f(0.25, 0.25, 0.0);
        glVertex3f(0.75, 0.25, 0.0);
        glVertex3f(0.75, 0.75, 0.0);
        glVertex3f(0.25, 0.75, 0.0);
    glEnd();

    tdGLSwapBuffers();
}
