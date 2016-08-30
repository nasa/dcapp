#ifndef _TARADRAW_HH_
#define _TARADRAW_HH_

#include "tdDefines.hh"

extern int tdButtLineCapStyle;
extern int tdRoundLineCapStyle;
extern int tdSquareLineCapStyle;
extern int tdAlignLeft;
extern int tdAlignCenter;
extern int tdAlignRight;
extern int tdAlignBottom;
extern int tdAlignMiddle;
extern int tdAlignTop;
extern int tdAlignBaseline;

// Initialization routines
extern int tdInitialize(char *);
extern tdWindow tdOpenWindow(const char *, float, float, float, float, int);
extern tdWindow tdOpenFullScreen(const char *);
extern tdImage *tdLoadImage(const char *, tdSize *);
extern tdGLContext *tdGLCreateContext(tdWindow);
extern void tdGLReshapeContext(float, float, int, float, float);
extern int tdRegisterColor(int, float, float, float);
extern int tdRegisterXFont(const char *, int, char *);

// Settings routines
extern void tdSetActiveWindow(tdWindow);
extern void tdGLSetContext(tdGLContext *);
extern void tdSetBackgroundColor(int);
extern void tdSetColor(int);
extern void tdSetColorRGB(float, float, float);
extern void tdSetFont(const char *, float);
extern void tdSetLineCap(int);
extern void tdSetLineWidth(float);
extern void tdSetNeedsRedraw(tdWindow);

// Query routines
extern void tdGetScreenData(tdSize *, tdRegion *);
extern tdColorRGB tdGetColor(int);
extern tdSize tdGetStringBounds(char *);
extern void tdGetPointer(tdPosition *, tdPosition *);
extern int tdNeedsRedraw(tdWindow);

// Drawing routines
extern void tdToggleFullScreen(void);
extern void tdClearWindow(void);
extern void tdDrawFilledPoly(float [], float [], int);
extern void tdDrawFilledRect(float, float, int, float, float);
extern void tdDrawString(char *, float, float, float, int);
extern void tdLineStart(float, float);
extern void tdLineAppend(float, float);
extern void tdDrawImage(tdImage *, float, float, int, float, float, float);
extern void tdGLSwapBuffers(void);
extern void tdRenderGraphics(void);

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
