#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include "varlist.hh"
#include "ccsds_udp_io.hh"
#include "CAN.hh"
#include "uei/UEI.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "msg.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

#define TIDY(a) if (a) { free(a); a=0x0; }
#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))

extern void mainloop(void);
extern void UpdateDisplay(void);
extern int update_dyn_elements(struct node *);
extern void SetNeedsRedraw(void);
extern void CheckMouseBounce(void);
extern void ui_init(char *);
extern void ui_terminate(void);
extern void appLauncher(char *, char **, char *, char **);
extern int ParseXMLFile(char *);

void Terminate(int);

static void ProcessArgs(int, char **);

appdata AppData;

static struct timeval last_ccsdsudp_try;


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
    static int ccsds_udp_active = 0;
    int status;
    struct timeval now;
    std::list<CommModule *>::iterator commitem;

    CAN_read();
    UEI_read();

    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        status = (*commitem)->write();
        if (status == CommModule::Terminate) Terminate(0);

        status = (*commitem)->read();
        if (status == CommModule::Success) UpdateDisplay();
        else if (status == CommModule::Terminate) Terminate(0);
    }

    if (ccsds_udp_active)
    {
    	// read:
    	CCSDS_INFO_RESULT errrorcause;
    	dcapp_ccsds_udp_io readstatus = ccsds_udp_readtlmmsg(&errrorcause);
		switch (readstatus)
		{
			case (CCSDSIO_INIT_ERROR):
			case (CCSDSIO_IO_ERROR):
			case (CCSDSIO_PARSE_ERROR):
				error_msg("ccsds_udp_readsimdata() had error= %d.", readstatus);
				error_msg("\t\t(error cause ID=%d)", errrorcause);
				ccsds_udp_active = 0; // stop reading
				break;
			case (CCSDSIO_NO_NEW_DATA):
				break; // ignore, nominal
			case (CCSDSIO_NOT_CCSDS):
				debug_msg("ccsds_udp_readsimdata() received non-CCSDS packet. status=%d", readstatus);
				debug_msg("\t\t(error cause ID=%d)", errrorcause);
				break; // ignore
			case (CCSDSIO_WRONG_CCSDS):
				debug_msg("ccsds_udp_readsimdata() received different CCSDS packet");
				debug_msg("\t\t(error cause ID=%d)", errrorcause);
				break; // ignore
			case (CCSDSIO_SUCCESS):
				UpdateDisplay();
				break;
			default:
				error_msg("ccsds reader switch status logic error, status=%d.", readstatus);
				ccsds_udp_active = 0; // stop reading
				break;
		}

    }
    else
    {
    	// attempt init:
        gettimeofday(&now, 0x0);
        if (SecondsElapsed(last_ccsdsudp_try, now) > CONNECT_ATTEMPT_INTERVAL)
        {
            if (ccsds_udp_finish_initialization() == CCSDSIO_SUCCESS)
            {
            	debug_msg("ccsds_udp opened socket for reading telemetry");
            	ccsds_udp_active = 1;
            }
            else
            {
            	error_msg("ccsds_udp failed to open socket for reading telemetry");
            }
            gettimeofday(&last_ccsdsudp_try, 0x0);
        }

        if (ccsds_udp_finish_send_initialization() == 0)
        {
        	debug_msg("ccsds_udp opened socket for sending commands");
        }
        else
        {
        	error_msg("ccsds_udp failed to open socket for sending commands");
        }
    }

    if (update_dyn_elements(AppData.window->p_current)) SetNeedsRedraw();

    CheckMouseBounce();

    gettimeofday(&now, 0x0);
    if (SecondsElapsed(AppData.last_update, now) > AppData.force_update) UpdateDisplay();
}


/*********************************************************************************
 *
 *  This cleanly shuts down the application.
 *
 *********************************************************************************/
void Terminate(int flag)
{
    std::list<CommModule *>::iterator commitem;

    AppData.DisplayClose();

    ui_terminate();
    CAN_term();

    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        delete (*commitem);
    }

    ccsds_udp_term();
    varlist_term();
    exit(flag);
}

static void ProcessArgs(int argc, char **argv)
{
    int i, count;
    char *xdisplay = 0x0;
    struct node *data;
    char *specfile = 0x0, *args = 0x0;
    char *name, *value;
    struct utsname minfo;
    size_t argsize;
    int gotargs = 0;

    uname(&minfo);

    if (argc < 2 && strcmp(minfo.sysname, "Darwin"))
    {
        user_msg("USAGE: dcapp <specfile> [optional arguments]");
        Terminate(-1);
    }

    /* Make sure that the following environment variables are set in case   */
    /* the user needs them: USER, LOGNAME, HOME, OSTYPE, MACHTYPE, and HOST */
    struct passwd *pw = getpwuid(getuid());
    char *lc_os = strdup(minfo.sysname);
    long hsize = sysconf(_SC_HOST_NAME_MAX)+1;
    char myhost[hsize];

    setenv("USER", pw->pw_name, 0);
    setenv("LOGNAME", pw->pw_name, 0);
    setenv("HOME", pw->pw_dir, 0);
    for (i=0; i<(int)strlen(lc_os); i++) lc_os[i] = tolower(lc_os[i]);
    setenv("OSTYPE", lc_os, 0);
    free(lc_os);
    setenv("MACHTYPE", minfo.machine, 0);
    if (!gethostname(myhost, hsize)) setenv("HOST", myhost, 0);

    if (argc > 2) gotargs = 1;
    else if (argc == 2 && strncmp(argv[1], "-psn", 4)) gotargs = 1;

    if (gotargs)
    {
        for (i=2; i<argc; i++)
        {
            if (argv[i][0] == '-' && argc > i+1)
            {
                if (!strcmp(argv[i], "-x")) xdisplay = strdup(argv[i+1]);
                i++;
            }
            else
            {
                if (args)
                {
                    argsize = strlen(args) + strlen(argv[i]) + 1;
                    args = (char *)realloc((void *)args, argsize+1);
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
        char *defspecfile = 0x0, *defargs = 0x0;
        char *appsupport, *preffilename;
        FILE *preffile;

        /* Get default arguments from the Application Support folder */
        asprintf(&appsupport, "%s/Library/Application Support/dcapp", getenv("HOME"));
        asprintf(&preffilename, "%s/Preferences", appsupport);

        preffile = fopen(preffilename, "r");
        if (preffile)
        {
            int retval;

            fseek(preffile, 0, SEEK_END);
            long len = ftell(preffile);
            rewind(preffile);
            defspecfile = (char *)malloc(len);
            defargs = (char *)malloc(len);
            retval = fscanf(preffile, "{%[^}]}", defspecfile);
            if (!retval) fseek(preffile, 1, SEEK_CUR);
            retval = fscanf(preffile, "{%[^}]}", defargs);
            fclose(preffile);
        }

        free(preffilename);
        free(appsupport);

        ui_init(0x0);
        appLauncher(defspecfile, &specfile, defargs, &args);

        TIDY(defspecfile);
        TIDY(defargs);
    }

    if (args)
    {
        char *strptr = args;

        name = (char *)calloc(sizeof(args), 1);
        value = (char *)calloc(sizeof(args), 1);

        while (strptr < (args + strlen(args)))
        {
            count = sscanf(strptr, "%[^= ]=%s", name, value);
            if (count == 2)
            {
                data = NewNode(0x0, &(AppData.ArgList));
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

    varlist_init();
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
            if (args) fprintf(preffile, "{%s}", args);
            else fprintf(preffile, "{}");
            fclose(preffile);
            free(fullpath);
        }

        free(preffilename);
        free(appsupport);
    }

    TIDY(specfile);
    TIDY(args);
}
