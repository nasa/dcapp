#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include "simio.h"
#include "trickio.h"
#include "edgeio.h"
#include "CAN.h"
#include "uei/UEI.h"
#include "nodes.h"
#include "string_utils.h"
#include "msg.h"

#define CONNECT_ATTEMPT_INTERVAL 2.0

#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))

extern void mainloop(void);
extern void UpdateDisplay(void);
extern int update_dyn_elements(struct node *);
extern void SetNeedsRedraw(void);
extern void ui_init(void);
extern void ui_terminate(void);

extern int ParseXMLFile(char *);

void Terminate(int);

static int ProcessArgs(int, char **);
static char *GetSpecfile(char *);

appdata AppData;

static struct timeval last_trickio_try, last_edgeio_try;


/*********************************************************************************
 *
 *  The main routine. Calls setup, handlers, and event loop.
 *
 *********************************************************************************/
int main(int argc, char **argv)
{
    if (ProcessArgs(argc, argv))
    {
        user_msg("USAGE: dcapp <specfile> [-h <hostname> -p <port> -d <display>]");
        Terminate(-1);
    }

    // Set up signal handlers
    signal(SIGINT, Terminate);
    signal(SIGTERM, Terminate);

    simio_initialize_parameter_list();
    ui_init();

    char *specfile = GetSpecfile(argv[1]);

    if (ParseXMLFile(specfile)) Terminate(-1);

    free(specfile);

    AppData.DisplayPreInit(get_pointer);
    AppData.DisplayInit();
    UpdateDisplay();

    // Set up timers
    gettimeofday(&last_trickio_try, NULL);
    gettimeofday(&last_edgeio_try, NULL);

    // Do forever
    mainloop();

    return(0);
}


/*********************************************************************************
 *
 *  This is where the application sits until it gets an event.
 *
 *********************************************************************************/
void Idle(void)
{
    static int trickio_active = 0, edgeio_active = 0;
    int status;
    struct timeval now;

    CAN_read();
    UEI_read();

    if (trickio_active)
    {
        status = trickio_readsimdata();

        if (status == SIMIO_SUCCESS) UpdateDisplay();
        else if (status == SIMIO_INVALID_CONNECTION) Terminate(0);
        else if (status == SIMIO_MANGLED_BUFFER) error_msg("Received a mangled data buffer");

        trickio_writesimdata();
    }
    else
    {
        gettimeofday(&now, NULL);
        if (SecondsElapsed(last_trickio_try, now) > CONNECT_ATTEMPT_INTERVAL)
        {
            if (trickio_activatecomm(AppData.simcomm.host, AppData.simcomm.port, AppData.simcomm.datarate) == SIMIO_SUCCESS) trickio_active = 1;
            gettimeofday(&last_trickio_try, NULL);
        }
    }

    if (edgeio_active)
    {
        status = edgeio_readsimdata();
        if (status == SIMIO_SUCCESS) UpdateDisplay();
        else if (status == SIMIO_ERROR) edgeio_active = 0;
        if (edgeio_writesimdata()) edgeio_active = 0;
    }
    else
    {
        gettimeofday(&now, NULL);
        if (SecondsElapsed(last_edgeio_try, now) > CONNECT_ATTEMPT_INTERVAL)
        {
            if (edgeio_activatecomm() == SIMIO_SUCCESS) edgeio_active = 1;
            gettimeofday(&last_edgeio_try, NULL);
        }
    }

    if (update_dyn_elements(AppData.window->p_current)) SetNeedsRedraw();

    gettimeofday(&now, NULL);
    if (SecondsElapsed(AppData.last_update, now) > AppData.force_update) UpdateDisplay();
}


/*********************************************************************************
 *
 *  This cleanly shuts down the application.
 *
 *********************************************************************************/
void Terminate(int flag)
{
    if (AppData.simcomm.host != NULL) free(AppData.simcomm.host);
    if (AppData.simcomm.datarate != NULL) free(AppData.simcomm.datarate);
    if (AppData.xdisplay != NULL) free(AppData.xdisplay);
    ui_terminate();
    CAN_term();
    trickio_term();
    edgeio_term();
    simio_term();
    exit(flag);
}

static int ProcessArgs(int argc, char **argv)
{
    int i, count;
    AppData.simcomm.host = 0;
    AppData.simcomm.port = 0;
    AppData.xdisplay = 0;
    struct node *data;
    char *name, *value;

    if (argc < 2) return (-1);

    for (i=2; i<argc; i++)
    {
        if (argv[i][0] == '-' && argc > i+1)
        {
            if (!strcmp(argv[i], "-h")) AppData.simcomm.host = strdup(argv[i+1]);
            if (!strcmp(argv[i], "-p")) AppData.simcomm.port = StrToInt(argv[i+1], 0);
            if (!strcmp(argv[i], "-d")) AppData.xdisplay = strdup(argv[i+1]);
            i++;
        }
        else
        {
            name = calloc(sizeof(argv[i]), 1);
            value = calloc(sizeof(argv[i]), 1);
            count = sscanf(argv[i], "%[^=]=%[^\n]", name, value);
            if (count == 2)
            {
                data = NewNode(NULL, &(AppData.ArgList));
                data->object.ppconst.name = strdup(name);
                data->object.ppconst.value = strdup(value);
            }
            free(name);
            free(value);
        }
    }

    return 0;
}

static char *GetSpecfile(char *arg)
{
    struct utsname minfo;

    uname(&minfo);

    if (strcmp(minfo.sysname, "Darwin")) return strdup(arg);

    char *appsupport, *preffilename, *retval = NULL;
    FILE *preffile;

    asprintf(&appsupport, "%s/Library/Application Support/dcapp", getenv("HOME"));
    asprintf(&preffilename, "%s/Preferences", appsupport);

    if (!strncmp(arg, "-psn", 4))
    {
        /* If launched as a double-clicked application... */
        char *trickhostcpu;
        int i;

        /* Set TRICK_HOST_CPU in case it's needed later */
        asprintf(&trickhostcpu, "%s_%s", minfo.sysname, minfo.release);
        for (i=0; i<strlen(trickhostcpu); i++)
        {
            if (trickhostcpu[i] == '.') trickhostcpu[i] = 0;
        }
        setenv("TRICK_HOST_CPU", trickhostcpu, 0);
        free(trickhostcpu);

        /* Get the specfile from the Application Support folder */
        preffile = fopen(preffilename, "r");
        if (preffile)
        {
            fseek(preffile, 0, SEEK_END);
            long len = ftell(preffile);
            rewind(preffile);
            retval = malloc(len+1);
            fscanf(preffile, "%s", retval);
            fclose(preffile);
        }
char *appLauncher(void);
retval = strdup(appLauncher());
    }
    else
    {
        /* If run from the command line, store the specfile for use the next time the app is double clicked */
        char *fullpath;
        mkdir(appsupport, 0755);
        preffile = fopen(preffilename, "w");
        if (preffile)
        {
            if (arg[0] == '/')
                asprintf(&fullpath, "%s", arg);
            else
                asprintf(&fullpath, "%s/%s", getcwd(0, 0), arg);
            fprintf(preffile, "%s", fullpath);
            fclose(preffile);
            free(fullpath);
        }
        retval = strdup(arg);
    }

    free(preffilename);
    free(appsupport);

    return retval;
}
