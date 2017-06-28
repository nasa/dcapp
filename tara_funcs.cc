#include "TaraDraw/TaraDraw.hh"

extern void Idle(void);
extern void Draw(void);
extern void Terminate(int);

extern void HandleMousePress(float, float);
extern void HandleMouseRelease(void);
extern void HandleKeyboard(unsigned char);
extern void UpdateStreams(void);
extern void reshape(int, int);

static void app_run(void);
static void app_term(void);
void mouse_click(ButtonEvent);
void key_click(KeyboardEvent);
void win_config(ConfigureEvent);
void win_close(WinCloseEvent);

static struct
{
    tdWindow windowID;      // changed to allow easier searching
    tdGLContext *gl_context;    // This does not need to be exposed at this level, every use is encapsulated in the drawing routines.
    float width;
    float height;
} mywin;


void ui_init(char *xdisplay)
{
    tdInitialize(xdisplay);
}


void window_init(bool fullscreen, int winOriginX, int winOriginY, int winSizeX, int winSizeY)
{
    tdRegion usable;

    tdGetScreenData(0x0, &usable);

    if (fullscreen)
    {
        mywin.windowID = tdOpenFullScreen("dcapp");
        mywin.width = usable.width;
        mywin.height = usable.height;
    }
    else
    {
        mywin.windowID = tdOpenWindow("dcapp", (float)winOriginX, usable.top - (float)winOriginY, (float)winSizeX, (float)winSizeY, tdAlignLeft | tdAlignTop);
        mywin.width = (float)winSizeX;
        mywin.height = (float)winSizeY;
    }

    mywin.gl_context = tdGLCreateContext(mywin.windowID);
}


void mouse_click(ButtonEvent btn)
{
    if (btn.state == tdPressed) HandleMousePress(btn.pos.x/mywin.width, btn.pos.y/mywin.height);
    else HandleMouseRelease();
}


void key_click(KeyboardEvent kbd)
{
    if (kbd.state == tdPressed && kbd.key) // Ignore if "specialkey" pressed
    {
        // Pressing 0x1b (escape) toggles fullscreen mode
        if (kbd.key == 0x1b) tdToggleFullScreen();
        // Change 0x7f (delete) to 0x08 (backspace) for standardization
        else if (kbd.key == 0x7f) HandleKeyboard(0x08);
        else HandleKeyboard(kbd.key);
    }
}


void win_config(ConfigureEvent cfg)
{
    reshape((int)(cfg.size.width), (int)(cfg.size.height));
    mywin.width = cfg.size.width;
    mywin.height = cfg.size.height;
    tdGLSetContext(mywin.gl_context);
    tdGLReshapeContext(0, 0, 0, cfg.size.width, cfg.size.height);
}


void win_close(WinCloseEvent wcl)
{
    app_term();
}


static void app_run(void)
{
    Idle();
    tdProcessEvents(mouse_click, key_click, win_config, win_close);
    UpdateStreams();
#ifndef IOS_BUILD
    if (tdNeedsRedraw(mywin.windowID))
        Draw();
#endif
}


static void app_term(void)
{
    Terminate(0);
}


void ui_terminate(void)
{
    tdTerminate();
}


void mainloop(void)
{
    tdMainLoop(app_run, app_term);
}


void SetNeedsRedraw(void)
{
    tdSetNeedsRedraw(mywin.windowID);
}


void SwapBuffers(void)
{
    tdGLSwapBuffers();
}
