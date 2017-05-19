#include <GL/glu.h>
#include "PixelStream/PixelStream.hh"
#include "basicutils/msg.hh"
#include "doug.h"
#include "dsp_plugin_common.h"

static PixelStreamData *psd = 0x0;

void PixelStream(void)
{
    if (psd)
    {
        GLint gldata[4];
        glGetIntegerv(GL_VIEWPORT, gldata);
        psd->width = (uint32_t)gldata[2];
        psd->height = (uint32_t)gldata[3];
        // TODO: Right now the plugin is reading the last viewport
        //       in the display.  We should look at adding an optional
        //       command-line argument to let you specify the display 
        //       to stream and a tcl command to let you switch it at
        //       runtime.  --FG

        //TODO: Should probably not make the glReadPixels call 
        //      if no clients are connected --FG
        glReadPixels(gldata[0], gldata[1], (GLsizei)(psd->width), (GLsizei)(psd->height), GL_RGB, GL_UNSIGNED_BYTE, psd->pixels);
        psd->writer();
    }

    // Execute rest of UPDATE_SCENE functional module
    DSF_ExecuteCore();
}

/* Initialize variables and install plugin function */
DSP_InitializePlugin(DSS_PLUGIN *plugin)
{
    Message::setLabel("PixelStream");
    unsigned protocol = PixelStreamFileProtocol;

    // Register plugin version
    DSF_RegisterPluginVersion(plugin, __DATE__ " - " __TIME__);

    if (plugin->argc < 3)
    {
        DSF_LogWarningQ("Not enough arguments to dsp_pixelstream.");
        DSF_LogWarningQ("Usage: -plugin dsp_pixelstream <TCP|filename> <port|shared_mem_id>");
        return 0;
    }
    if (!strcasecmp(plugin->argv[1], "TCP")) protocol = PixelStreamTcpProtocol;

    PixelStreamFile *psf;
    PixelStreamTcp *pst;

    int retval = 0;
    switch (protocol)
    {
        case PixelStreamFileProtocol:
            psf = new PixelStreamFile;
            retval = psf->writerInitialize(plugin->argv[1], strtol(plugin->argv[2], 0x0, 10));
            psd = (PixelStreamData *)psf;
            break;
        case PixelStreamTcpProtocol:
            pst = new PixelStreamTcp;
            retval = pst->writerInitialize(strtol(plugin->argv[2], 0x0, 10));
            psd = (PixelStreamData *)pst;
            break;
        default:
            return 0;
    }

    if (retval < 0)
    {
        DSF_LogWarningQ("Error initializing dsp_pixelstream plugin.");
        return 0;
    }
    else if (protocol == PixelStreamFileProtocol)
    {
        DSF_LogStatusQ("dsp_pixelstream now streaming to file at %s with shmid %ld", plugin->argv[1], strtol(plugin->argv[2], 0x0, 10));
    }
    else if (protocol == PixelStreamTcpProtocol)
    {
        DSF_LogStatusQ("dsp_pixelstream now streaming over TCP on port %ld", strtol(plugin->argv[2], 0x0, 10));
    }

    // Set mem allocation based upon GL_MAX_VIEWPORT_DIMS because dynamic method based upon GL_VIEWPORT isn't reliable
    GLint gldata[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, gldata);
    psd->pixels = malloc((size_t)(gldata[0] * gldata[1]));

    // Make sure that glReadPixels uses byte alignment, otherwise images may appear skewed
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Install plugin function - PixelStream - in UPDATE_DISPLAY functional module
    DSF_InstallPluginFunction(plugin->handle, PixelStream, DSD_PLUGIN_UPDATE_DISPLAY);

    return 1;
}

/* Cleanup function upon exiting DOUG */
DSP_ExitPlugin(DSS_PLUGIN *plugin)
{
    if (psd) delete psd;
}
