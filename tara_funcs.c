#include <stdio.h>
#include "TaraDraw/TaraDraw.h"
#include "mappings.h"

extern void Idle(void);
extern void Draw(void);
extern void Terminate(int);

extern void HandleMouse(int, int, float, float, int);
extern void HandleKeyboard(unsigned char, int, int);
extern void HandleSpecialKeyboard(int, int, int);
extern void reshape(int, int);

static void app_run(void);
static void app_term(void);
void mouse_click(ButtonEvent);
void key_click(KeyboardEvent);
void win_config(ConfigureEvent);
void win_close(WinCloseEvent);

static struct
{
    tdWindow id;
    tdGLContext *gl_context;
    float width;
    float height;
} mywin;


void ui_init(char *xdisplay)
{
    tdInitialize(xdisplay);
}


void window_init(int fullscreen, int winOriginX, int winOriginY, int winSizeX, int winSizeY)
{
    tdRegion usable;

    tdGetScreenData(NULL, &usable);

    if (fullscreen)
    {
        mywin.id = tdOpenFullScreen("dcapp");
        mywin.width = usable.width;
        mywin.height = usable.height;
    }
    else
    {
        mywin.id = tdOpenWindow("dcapp", (float)winOriginX, usable.top - (float)winOriginY, (float)winSizeX, (float)winSizeY, tdAlignLeft | tdAlignTop);
        mywin.width = (float)winSizeX;
        mywin.height = (float)winSizeY;
    }

    mywin.gl_context = tdGLCreateContext(mywin.id);
}


void toggle_fullscreen(void)
{
    tdToggleFullScreen();
}


void mouse_click(ButtonEvent btn)
{
    if (btn.state == tdPressed) HandleMouse(0, MOUSE_DOWN, btn.pos.x/mywin.width, btn.pos.y/mywin.height, 0);
    else HandleMouse(0, MOUSE_UP, 0, 0, 0);
}


void key_click(KeyboardEvent kbd)
{
    if (kbd.state == tdPressed)
    {
        if (kbd.specialkey)
        {
            switch (kbd.specialkey)
            {
                case tdArrowLeft:
                    HandleSpecialKeyboard(KEY_LEFT, kbd.pos.x, kbd.pos.y);
                    break;
                case tdArrowRight:
                    HandleSpecialKeyboard(KEY_RIGHT, kbd.pos.x, kbd.pos.y);
                    break;
                case tdArrowDown:
                    HandleSpecialKeyboard(KEY_DOWN, kbd.pos.x, kbd.pos.y);
                    break;
                case tdArrowUp:
                    HandleSpecialKeyboard(KEY_UP, kbd.pos.x, kbd.pos.y);
                    break;
            }
        }
        else
        {
            if (kbd.key == 0x7f) HandleKeyboard(0x08, kbd.pos.x, kbd.pos.y);
            else HandleKeyboard(kbd.key, kbd.pos.x, kbd.pos.y);
        }
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
    if (tdNeedsRedraw(mywin.id)) Draw();
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
