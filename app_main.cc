#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cctype>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include "basicutils/msg.hh"
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "basicutils/pathinfo.hh"
#include "basicutils/shellutils.hh"
#include "basicutils/stringutils.hh"
#include "comm.hh"
#include "device.hh"
#include "app_data.hh"
#include "xml_stringsub.hh"
#include "osenv/osenv.hh"
#include "PixelStream/curlLib.hh"

#define MINIMUM_REFRESH 0.05 // 50 ms = 20 Hz

extern void mainloop(void);
extern void UpdateDisplay(void);
extern void SetNeedsRedraw(void);
extern void CheckMouseBounce(void);
extern void ui_init(char *);
extern void ui_terminate(void);
extern int ParseXMLFile(const char *);
extern void DisplayPreInitStub(void *(*)(const char *));
extern void DisplayInitStub(void);
extern void DisplayLogicStub(void);
extern void DisplayCloseStub(void);

void Terminate(int);

static void SetDefaultEnvironment(std::string);
static void ProcessArgs(int, char **);

appdata AppData;

int main(int argc, char **argv)
{
    // Set up signal handlers
    signal(SIGINT, Terminate);
    signal(SIGTERM, Terminate);
    signal(SIGPIPE, SIG_IGN);

    Message::setLabel(basename(argv[0]));
#ifdef DEBUG
    Message::enableDebugging();
#endif

    AppData.master_timer = new Timer;
    AppData.last_update = new Timer;

    AppData.DisplayPreInit = &DisplayPreInitStub;
    AppData.DisplayInit = &DisplayInitStub;
    AppData.DisplayLogic = &DisplayLogicStub;
    AppData.DisplayClose = &DisplayCloseStub;

    AppData.toplevel = 0;
    SetDefaultEnvironment(argv[0]);
    ProcessArgs(argc, argv);
    if (!(AppData.toplevel))
    {
        error_msg("A Window element must be defined in the specfile");
        Terminate(0);
    }

    curlLibInit();
    AppData.DisplayPreInit(get_pointer);
    AppData.DisplayInit();
    UpdateDisplay();

    // Do forever
    mainloop();

    return(0);
}

void Idle(void)
{
    static Timer *mytime = new Timer;
    int status;
    std::list<DeviceModule *>::iterator deviceitem;
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;

    float elapsed = mytime->getSeconds();
    mytime->restart();

    // hibernate until the next period of time defined by MINIMUM_REFRESH to temporarily release the CPU
    if (elapsed < MINIMUM_REFRESH) hibernate(MINIMUM_REFRESH - elapsed);

    for (deviceitem = AppData.devicelist.begin(); deviceitem != AppData.devicelist.end(); deviceitem++)
    {
        (*deviceitem)->read();
    }

    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        status = (*commitem)->write();
        if (status == CommModule::Terminate) Terminate(0);

        status = (*commitem)->read();
        if (status == CommModule::Success) UpdateDisplay();
        else if (status == CommModule::Terminate) Terminate(0);

        (*commitem)->updateConnectedVariable();
    }

    if (!AppData.animators.empty())
    {
        SetNeedsRedraw();
        for (animitem = AppData.animators.begin(); animitem != AppData.animators.end(); animitem++)
        {
            if ((*animitem)->update(AppData.master_timer->getSeconds()))
            {
                delete *animitem;
                *animitem = 0x0;
            }
        }
        AppData.animators.remove(0x0);
    }

    CheckMouseBounce();

    if (AppData.last_update->getSeconds() > AppData.force_update) UpdateDisplay();
}

void Terminate(int flag)
{
    std::list<DeviceModule *>::iterator deviceitem;
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;
    std::list<PixelStreamItem *>::iterator psitem;

    if (AppData.DisplayClose) AppData.DisplayClose();

    ui_terminate();

    for (deviceitem = AppData.devicelist.begin(); deviceitem != AppData.devicelist.end(); deviceitem++)
    {
        delete (*deviceitem);
    }
    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        delete (*commitem);
    }
    for (psitem = AppData.pixelstreams.begin(); psitem != AppData.pixelstreams.end(); psitem++)
    {
        delete (*psitem);
    }
    for (animitem = AppData.animators.begin(); animitem != AppData.animators.end(); animitem++)
    {
        delete (*animitem);
    }

    curlLibTerm();
    exit(flag);
}

/* Set the dcappOSTYPE, dcappOSSPEC, dcappOBJDIR, and dcappBINDIR environment       */
/* variables in case the user needs them.  Likewise, set the following for the user */
/* if they have not been set: USER, LOGNAME, HOME, OSTYPE, MACHTYPE, and HOST.      */
static void SetDefaultEnvironment(std::string pathspec)
{
    unsigned i;
    struct utsname minfo;
    struct passwd *pw = getpwuid(getuid());
    long hsize = sysconf(_SC_HOST_NAME_MAX)+1;
    char myhost[hsize];

    std::string mypath = findExecutablePath(pathspec);

    AppData.dcapphome = PathInfo(mypath + "/../../../").getFullPath();
    AppData.defaultfont = AppData.dcapphome + "/dcapp.app/Contents/Resources/fonts/defaultfont";

    std::string configscript = PathInfo(mypath + "/../dcapp-config").getFullPath();

    setenv("dcappOSTYPE", getScriptResult(configscript, "--ostype").c_str(), 1);
    setenv("dcappOSSPEC", getScriptResult(configscript, "--osspec").c_str(), 1);
    setenv("dcappOBJDIR", getScriptResult(configscript, "--objdir").c_str(), 1);
    setenv("dcappBINDIR", getScriptResult(configscript, "--bindir").c_str(), 1);
    setenv("dcappVERSION", getScriptResult(configscript, "--version_short").c_str(), 1);

    uname(&minfo);

    std::string lcos, os = minfo.sysname;
    for (i = 0; i < os.length(); i++) lcos += tolower(os[i]);
    setenv("OSTYPE", lcos.c_str(), 0);

    setenv("USER", pw->pw_name, 0);
    setenv("LOGNAME", pw->pw_name, 0);
    setenv("HOME", pw->pw_dir, 0);
    setenv("MACHTYPE", minfo.machine, 0);

    if (!gethostname(myhost, hsize)) setenv("HOST", myhost, 0);
}

static void ProcessArgs(int argc, char **argv)
{
    int i, count, gotargs;
    char *xdisplay = 0x0, *specfile = 0x0, *args = 0x0;
    size_t argsize;

    gotargs = checkArgs(argc, argv);

    switch (gotargs)
    {
        case 1:
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
            break;
        case 0:
            ui_init(0x0);
            getArgs(&specfile, &args);
            break;
        default:
            user_msg("USAGE: dcapp <specfile> [optional arguments]");
            Terminate(-1);
    }

    if (args)
    {
        char *strptr = args;
        char *key = (char *)calloc(strlen(args), 1);
        char *value = (char *)calloc(strlen(args), 1);

        while (strptr < (args + strlen(args)))
        {
            count = sscanf(strptr, "%[^= ]=%s", key, value);

            if (count == 1 && !strcmp(key, "-debug")) Message::enableDebugging();
            else if (count == 2) processArgument(key, value);

            if (count >= 1) strptr += strlen(key);
            if (count == 2) strptr += strlen(value) + 1;

            while (strptr < (args + strlen(args)) && *strptr == ' ') strptr++;
        }

        free(key);
        free(value);
    }

    if (ParseXMLFile(specfile)) Terminate(-1);

    storeArgs(specfile, args);

    TIDY(specfile);
    TIDY(args);
}
