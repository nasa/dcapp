#ifndef _TARADRAW_HH_
#define _TARADRAW_HH_

#include <string>
#include "tdDefines.hh"

extern int tdAlignLeft;
extern int tdAlignCenter;
extern int tdAlignRight;
extern int tdAlignBottom;
extern int tdAlignMiddle;
extern int tdAlignTop;

// Initialization routines
extern int tdInitialize(const std::string &);
extern tdWindow tdOpenWindow(const std::string &, float, float, float, float, int);
extern tdWindow tdOpenFullScreen(const std::string &);
extern tdGLContext *tdGLCreateContext(tdWindow);
extern void tdGLReshapeContext(float, float, int, float, float);

// Settings routines
extern void tdSetActiveWindow(tdWindow);
extern void tdGLSetContext(tdGLContext *);
extern void tdSetNeedsRedraw(tdWindow);

// Query routines
extern void tdGetScreenData(tdSize *, tdRegion *);
extern void tdGetPointer(tdPosition *, tdPosition *);
extern int tdNeedsRedraw(tdWindow);

// Drawing routines
extern void tdToggleFullScreen(void);
extern void tdGLSwapBuffers(void);

// Event processing
extern void tdMainLoop(void (*)(void), void (*)(void));
extern void tdProcessEvents(void (*)(ButtonEvent),
                            void (*)(KeyboardEvent),
                            void (*)(ConfigureEvent),
                            void (*)(WinCloseEvent));

// Termination routines
extern void tdCloseWindow(void);
extern void tdGLDestroyContext(void);
extern void tdTerminate(void);

#endif
