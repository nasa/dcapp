#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <GL/glu.h>
#include "utils/msg.hh"
#include "TaraDraw/TaraDraw.hh"
#include "PixelStream/PixelStream.hh"

static tdWindow winid;
static float winwidth = 640, winheight = 480;
static tdGLContext *gl_context;
static PixelStreamData *psd;
static unsigned int textureID;
static void *pixels;
static size_t memallocation = 0;

void Draw(void)
{
	size_t nbytes, origbytes, newh, pad, padbytes, offset, offsetbytes;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winwidth, 0, winheight, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    newh = (size_t)((float)(psd->width) * winheight / winwidth);
    if (newh > psd->height)
    {
        origbytes = psd->width * psd->height * 3;
        // overpad the pad by 1 so that no unfilled rows show up
        pad = ((newh - psd->height) / 2) + 1;
        padbytes = psd->width * pad * 3;
        nbytes = (size_t)(psd->width * newh * 3);
        if (nbytes > memallocation)
        {
            pixels = realloc(pixels, nbytes);
            memallocation = nbytes;
        }
        bzero(pixels, padbytes);
        bzero((void *)((size_t)pixels + nbytes - padbytes), padbytes);
        bcopy(psd->pixels, (void *)((size_t)pixels + padbytes), origbytes);
    }
    else
    {
        nbytes = (size_t)(psd->width * psd->height * 3);
        if (nbytes > memallocation)
        {
            pixels = realloc(pixels, nbytes);
            memallocation = nbytes;
        }
        bcopy(psd->pixels, pixels, nbytes);
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if (newh < psd->height)
    {
        offset = (psd->height - newh) / 2;
        offsetbytes = psd->width * offset * 3;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, psd->width, newh, 0, GL_RGB, GL_UNSIGNED_BYTE, (void *)((size_t)pixels + offsetbytes));
    }
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, psd->width, newh, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(0, 0, 0);
        glTexCoord2f(1, 0);
        glVertex3f(winwidth, 0, 0);
        glTexCoord2f(1, 1);
        glVertex3f(winwidth, winheight, 0);
        glTexCoord2f(0, 1);
        glVertex3f(0, winheight, 0);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

static void app_term(void)
{
    tdTerminate();
    exit(0);
}

void win_config(ConfigureEvent cfg)
{
    winwidth = cfg.size.width;
    winheight = cfg.size.height;
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, (int)(winwidth), (int)(winheight));
    tdGLReshapeContext(0, 0, 0, winwidth, winheight);
}

void win_close(WinCloseEvent wcl)
{
    app_term();
}

static void app_run(void)
{
    tdProcessEvents(0x0, 0x0, win_config, win_close);
    if (psd->reader()) tdSetNeedsRedraw(winid);
    if (tdNeedsRedraw(winid))
    {
        Draw();
        tdGLSwapBuffers();
    }
}

void usageError(const char *appname)
{
    printf("usage:\n  %s filename shared_memory_key\n  %s TCP <host> port\n", appname, appname);
    exit(0);
}

int main(int argc, char **argv)
{
    Message::setLabel(argv[0]);

    if (argc < 3 || argc > 4) usageError(argv[0]);

    if (!strcmp(argv[1], "TCP"))
    {
        char *host;
        int port;

        if (argc == 3)
        {
            host = strdup("localhost");
            port = strtol(argv[2], 0x0, 10);
        }
        else
        {
            host = strdup(argv[2]);
            port = strtol(argv[3], 0x0, 10);
        }        

        PixelStreamTcp *pst = new PixelStreamTcp;
        if (pst->readerInitialize(host, port)) return -1;
        psd = (PixelStreamData *)pst;
        psd->protocol = PixelStreamTcpProtocol;
    }
    else
    {
        char *filename;
        int shared_memory_key;

        if (argc == 4) usageError(argv[0]);

        filename = strdup(argv[1]);
        shared_memory_key = strtol(argv[2], 0x0, 10);

        PixelStreamFile *psf = new PixelStreamFile;
        if (psf->initialize(filename, shared_memory_key, 0)) return -1;
        psd = (PixelStreamData *)psf;
        psd->protocol = PixelStreamFileProtocol;
    }

    tdInitialize(0x0);
    winid = tdOpenWindow(argv[0], 100, 100, winwidth, winheight, tdAlignLeft | tdAlignBottom);
    gl_context = tdGLCreateContext(winid);

    glClearColor(0, 0, 0, 0);
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);

    glGenTextures(1, (GLuint *)(&textureID));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    tdMainLoop(app_run, app_term);

    return 0;
}
