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

#define TIDY(a) if (a != NULL) { free(a); a=NULL; }
#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))

extern void mainloop(void);
extern void UpdateDisplay(void);
extern int update_dyn_elements(struct node *);
extern void SetNeedsRedraw(void);
extern void CheckMouseBounce(void);
extern void ui_init(char *);
extern void ui_terminate(void);
extern void appLauncher(char *, char **, char *, char **, char *, char **, char *, char **);
extern int ParseXMLFile(char *);

void Terminate(int);

static void ProcessArgs(int, char **);

appdata AppData;

static struct timeval last_trickio_try, last_edgeio_try;


/*********************************************************************************
 *
 *  The main routine. Calls setup, handlers, and event loop.
 *
 *********************************************************************************/
int main(int argc, char **argv)
{
    // Set up signal handlers
    signal(SIGINT, Terminate);
    signal(SIGTERM, Terminate);

    ProcessArgs(argc, argv);

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
        else if (status == SIMIO_INVALID_CONNECTION)
        {
            user_msg("TrickComm connection terminated");
            Terminate(0);
        }
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

    CheckMouseBounce();

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
    TIDY(AppData.simcomm.host);
    TIDY(AppData.simcomm.datarate);

    ui_terminate();
    CAN_term();
    trickio_term();
    edgeio_term();
    simio_term();
    exit(flag);
}

static void ProcessArgs(int argc, char **argv)
{
    int i, count;
    AppData.simcomm.host = 0;
    AppData.simcomm.port = 0;
    char *xdisplay = NULL;
    struct node *data;
    char *specfile = NULL, *host = NULL, *port = NULL, *args = NULL;
    char *name, *value;
    struct utsname minfo;
    int argsize, gotargs = 0;

    uname(&minfo);

    if (argc < 2 && strcmp(minfo.sysname, "Darwin"))
    {
        user_msg("USAGE: dcapp <specfile> [-h <hostname> -p <port> -d <display>]");
        Terminate(-1);
    }

    if (argc > 2) gotargs = 1;
    else if (argc == 2 && strncmp(argv[1], "-psn", 4)) gotargs = 1;

    if (gotargs)
    {
        for (i=2; i<argc; i++)
        {
            if (argv[i][0] == '-' && argc > i+1)
            {
                if (!strcmp(argv[i], "-h")) host = strdup(argv[i+1]);
                if (!strcmp(argv[i], "-p")) port = strdup(argv[i+1]);
                if (!strcmp(argv[i], "-d")) xdisplay = strdup(argv[i+1]);
                i++;
            }
            else
            {
                if (args)
                {
                    argsize = strlen(args) + strlen(argv[i]) + 1;
                    args = realloc(args, argsize+1);
                    sprintf(args, "%s %s", args, argv[i]);
                    args[argsize] = '\0';
                }
                else args = strdup(argv[i]);
            }
        }

        specfile = strdup(argv[1]);
        ui_init(xdisplay);
        TIDY(xdisplay);
    }
    else /* We're on a Mac and the command line arguments don't provide enough information to proceed... */
    {
        char *defspecfile = NULL, *defhost = NULL, *defport = NULL, *defargs = NULL;

        /* Set TRICK_HOST_CPU in case it's needed later */
        char *trickhostcpu;
        int i;

        asprintf(&trickhostcpu, "%s_%s", minfo.sysname, minfo.release);
        for (i=0; i<strlen(trickhostcpu); i++)
        {
            if (trickhostcpu[i] == '.') trickhostcpu[i] = 0;
        }
        setenv("TRICK_HOST_CPU", trickhostcpu, 0);
        free(trickhostcpu);

        /* Get default arguments from the Application Support folder */
        char *appsupport, *preffilename;
        FILE *preffile;

        asprintf(&appsupport, "%s/Library/Application Support/dcapp", getenv("HOME"));
        asprintf(&preffilename, "%s/Preferences", appsupport);

        preffile = fopen(preffilename, "r");
        if (preffile)
        {
            int retval;

            fseek(preffile, 0, SEEK_END);
            long len = ftell(preffile);
            rewind(preffile);
            defspecfile = malloc(len);
            defhost = malloc(len);
            defport = malloc(len);
            defargs = malloc(len);
            retval = fscanf(preffile, "{%[^}]}", defspecfile);
            if (!retval) fseek(preffile, 1, SEEK_CUR);
            retval = fscanf(preffile, "{%[^}]}", defhost);
            if (!retval) fseek(preffile, 1, SEEK_CUR);
            retval = fscanf(preffile, "{%[^}]}", defport);
            if (!retval) fseek(preffile, 1, SEEK_CUR);
            retval = fscanf(preffile, "{%[^}]}", defargs);
            fclose(preffile);
        }

        free(preffilename);
        free(appsupport);

        ui_init(NULL);
        appLauncher(defspecfile, &specfile, defhost, &host, defport, &port, defargs, &args);

        TIDY(defspecfile);
        TIDY(defhost);
        TIDY(defport);
        TIDY(defargs);
    }

    if (host) AppData.simcomm.host = strdup(host);
    AppData.simcomm.port = StrToInt(port, 0);
    if (args)
    {
        char *strptr = args;

        name = calloc(sizeof(args), 1);
        value = calloc(sizeof(args), 1);

        while (strptr < (args + strlen(args)))
        {
            count = sscanf(strptr, "%[^= ]=%s", name, value);
            if (count == 2)
            {
                data = NewNode(NULL, &(AppData.ArgList));
                data->object.ppconst.name = strdup(name);
                data->object.ppconst.value = strdup(value);
            }
            if (count >= 1) strptr += strlen(name);
            if (count == 2) strptr += strlen(value) + 1;
            while (strptr < (args + strlen(args)) && *strptr == ' ') strptr++;
        }

        free(name);
        free(value);
    }

    simio_initialize_parameter_list();
    if (ParseXMLFile(specfile)) Terminate(-1);

    /* On the Mac, store arguments to use as defaults the next time the app is run */
    if (!strcmp(minfo.sysname, "Darwin"))
    {
        char *appsupport, *preffilename;
        FILE *preffile;

        asprintf(&appsupport, "%s/Library/Application Support/dcapp", getenv("HOME"));
        asprintf(&preffilename, "%s/Preferences", appsupport);

        char *fullpath;
        mkdir(appsupport, 0755);
        preffile = fopen(preffilename, "w");
        if (preffile)
        {
            if (specfile[0] == '/')
                asprintf(&fullpath, "%s", specfile);
            else
                asprintf(&fullpath, "%s/%s", getcwd(0, 0), specfile);
            fprintf(preffile, "{%s}", fullpath);
            if (host) fprintf(preffile, "{%s}", host);
            else fprintf(preffile, "{}");
            if (port) fprintf(preffile, "{%s}", port);
            else fprintf(preffile, "{}");
            if (args) fprintf(preffile, "{%s}", args);
            else fprintf(preffile, "{}");
            fclose(preffile);
            free(fullpath);
        }

        free(preffilename);
        free(appsupport);
    }

    TIDY(specfile);
    TIDY(host);
    TIDY(port);
    TIDY(args);
}
