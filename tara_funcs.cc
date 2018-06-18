#include "TaraDraw/TaraDraw.hh"
#include "nodes.hh"
#include "varlist.hh"
#include "PixelStream/curlLib.hh"

extern appdata AppData;

extern void Idle(void);
extern void Terminate(int);

extern void HandleMouseMotion(double, double);
extern void HandleMousePress(double, double);
extern void HandleMouseRelease(void);
extern void HandleKeyboard(unsigned char);
extern void reshape_viewport(int, int);

static void app_run(void);
static void app_term(void);
void mouse_click(ButtonEvent);
void key_click(KeyboardEvent);
void win_config(ConfigureEvent);
void win_close(WinCloseEvent);
void SwapBuffers(void);

static struct
{
    tdWindow id;
    tdGLContext *gl_context;
    double width;
    double height;
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
        mywin.id = tdOpenFullScreen("dcapp");
        mywin.width = usable.width;
        mywin.height = usable.height;
    }
    else
    {
        mywin.id = tdOpenWindow("dcapp", (float)winOriginX, usable.top - (float)winOriginY, (float)winSizeX, (float)winSizeY, tdAlignLeft | tdAlignTop);
        mywin.width = (double)winSizeX;
        mywin.height = (double)winSizeY;
    }

    mywin.gl_context = tdGLCreateContext(mywin.id);
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
    reshape_viewport((int)(cfg.size.width), (int)(cfg.size.height));
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
    static unsigned passnum = 0;
    tdPosition pos;
    std::list<PixelStreamItem *>::iterator psitem;

    passnum++;
    Idle();
    tdGetPointer(nullptr, &pos);
    HandleMouseMotion(pos.x/mywin.width, pos.y/mywin.height);
    tdProcessEvents(mouse_click, key_click, win_config, win_close);

    for (psitem = AppData.pixelstreams.begin(); psitem != AppData.pixelstreams.end(); psitem++)
        (*psitem)->psd->updateStatus();
    curlLibRun();
    AppData.toplevel->updateStreams(passnum);

    if (tdNeedsRedraw(mywin.id))
    {
        AppData.toplevel->draw();
        SwapBuffers();
        AppData.last_update->restart();
    }
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
    tdSetNeedsRedraw(mywin.id);
}


void SwapBuffers(void)
{
    tdGLSwapBuffers();
}


void UpdateDisplay(void)
{
    AppData.toplevel->setCurrentPanel();
    AppData.DisplayLogic();
    SetNeedsRedraw();
}
