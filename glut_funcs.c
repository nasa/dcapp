#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <GL/glut.h>
#include "mappings.h"

extern void Draw(void);
extern void Idle(void);
extern void Terminate(int);
extern void HandleKeyboard(unsigned char, int, int);
extern void HandleMouse(int, int, float, float, int);
extern void HandleMouseMotion(int, int);
extern void HandlePassiveMotion(int, int);
extern void HandleSpecialKeyboard(int, int, int);
extern void reshape(int, int);

static void HandleGLUTReshape(int, int);
static void HandleGLUTMenu(int);
static void HandleGLUTSpecialKeyboard(int, int, int);
static void HandleGLUTMouse(int ,int ,int ,int );
static void HandleGLUTVisibility(int);
static void HandleGLUTEntry(int);

static struct
{
    float width;
    float height;
} mywin;


void ui_init(char *xDisplay)
{
    static int myargc;
    static char *myargv[3];

    // Fake out args to glutInit so that we don't have to pass argc and argv through init routines
    if (xDisplay)
    {
        myargc = 3;
        myargv[0] = strdup("dcapp");
        myargv[1] = strdup("-display");
        myargv[2] = strdup(xDisplay);
    }
    else
    {
        myargc = 1;
        myargv[0] = strdup("dcapp");
    }

    glutInit(&myargc, myargv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
}


void window_init(int fullscreen, int winOriginX, int winOriginY, int winSizeX, int winSizeY)
{
    if (!fullscreen)
    {
        glutInitWindowPosition(winOriginX, winOriginY);
        glutInitWindowSize(winSizeX, winSizeY);
        mywin.width = (float)winSizeX;
        mywin.height = (float)winSizeY;
    }

    glutCreateWindow("dcapp");

    if (fullscreen)
    {
        glutFullScreen();
        mywin.width = (float)glutGet(GLUT_SCREEN_WIDTH);
        mywin.height = (float)glutGet(GLUT_SCREEN_HEIGHT);
    }

    glutDisplayFunc(Draw);
    glutEntryFunc(HandleGLUTEntry);
    glutReshapeFunc(HandleGLUTReshape);
    glutVisibilityFunc(HandleGLUTVisibility);
    glutKeyboardFunc(HandleKeyboard);
    glutSpecialFunc(HandleGLUTSpecialKeyboard);
    glutMouseFunc(HandleGLUTMouse);
    glutMotionFunc(HandleMouseMotion);
    glutPassiveMotionFunc(HandlePassiveMotion);

    /* Set up the main menu */
    glutCreateMenu(HandleGLUTMenu);
    glutAddMenuEntry("Quit", 1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void mainloop(void)
{
    glutMainLoop();
}


void ui_terminate(void)
{
}


// Since GLUT can't toggle full-screen, just terminate since there's no other way to kill a full-screen window
void toggle_fullscreen(void)
{
    Terminate(0);
}


/*********************************************************************************
 *
 *  This function is called when a window is made visible or when it's made invisible
 *
 *********************************************************************************/
static void HandleGLUTVisibility(int visible)
{
    if (visible == GLUT_VISIBLE) glutIdleFunc(Idle);
    else glutIdleFunc(NULL);
}


/*********************************************************************************
 *
 *  This is where the application posts the event to redraw the window.
 *
 *********************************************************************************/
void SetNeedsRedraw(void)
{
    glutPostRedisplay();
}


/*********************************************************************************
 *
 *  This is where the application posts the event to handle entry into a window.
 *
 *********************************************************************************/
static void HandleGLUTEntry(int state)
{
    if (state == GLUT_ENTERED) glutSetWindow(glutGetWindow());
}


/*********************************************************************************
 *
 *  Make the previously drawn to graphics buffer visible, while rotating the visible
 *  one to the back so that it can be drawn.
 *
 *********************************************************************************/
void SwapBuffers(void)
{
    glutSwapBuffers();
}


/*********************************************************************************
 *
 *  Handle keyboard events like the arrow keys and function keys.
 *
 *********************************************************************************/
static void HandleGLUTSpecialKeyboard(int glutkey, int x, int y)
{
    int key;

    switch (glutkey)
    {
        case GLUT_KEY_LEFT:
            key = KEY_LEFT;
            break;
        case GLUT_KEY_RIGHT:
            key = KEY_RIGHT;
            break;
        case GLUT_KEY_UP:
            key = KEY_UP;
            break;
        case GLUT_KEY_DOWN:
            key = KEY_DOWN;
            break;
        default:
            key = UNKNOWN;
            break;
    }

    HandleSpecialKeyboard(key, x, y);
}


static void HandleGLUTReshape(int w, int h)
{
    reshape(w, h);
    mywin.width = (float)w;
    mywin.height = (float)h;
}


static void HandleGLUTMenu(int item)
{
    if (item == 1) Terminate(0);
}


/*********************************************************************************
 *
 *  Handle mouse events (up, down, etc.)
 *
 *********************************************************************************/
static void HandleGLUTMouse(int glutbutton, int glutstate, int x, int y)
{
    int state, button, modifier, glutmodifier;

    switch (glutstate)
    {
        case GLUT_DOWN:
            state = MOUSE_DOWN;
            break;
        case GLUT_UP:
            state = MOUSE_UP;
            break;
        default:
            state = UNKNOWN;
            break;
    }

    switch (glutbutton)
    {
        case GLUT_LEFT_BUTTON:
            button = LEFT_BUTTON;
            break;
        case GLUT_MIDDLE_BUTTON:
            button = MIDDLE_BUTTON;
            break;
        case GLUT_RIGHT_BUTTON:
            button = RIGHT_BUTTON;
            break;
        default:
            button = UNKNOWN;
            break;
    }

    glutmodifier = glutGetModifiers();
    switch (glutmodifier)
    {
        case GLUT_ACTIVE_SHIFT:
            modifier = SHIFT;
            break;
        case GLUT_ACTIVE_CTRL:
            modifier = CTRL;
            break;
        case GLUT_ACTIVE_ALT:
            modifier = ALT;
            break;
        default:
            modifier = UNKNOWN;
            break;
    }

    HandleMouse(button, state, (float)x/mywin.width, 1 - (float)y/mywin.height, modifier);
}
