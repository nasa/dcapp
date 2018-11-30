#include <vector>
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
#include "basicutils/pathinfo.hh"
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "varlist.hh"
#include "can/CAN.hh"
#include "uei/UEI.hh"
#include "nodes.hh"
#include "string_utils.hh"
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

    SetDefaultEnvironment(argv[0]);
    ProcessArgs(argc, argv);

    curlLibInit();
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
    static Timer *mytime = new Timer;
    int status;
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;

    float elapsed = mytime->getSeconds();
    mytime->restart();

    // usleep to the next period of time defined by MINIMUM_REFRESH to temporarily release the CPU
    if (elapsed < MINIMUM_REFRESH) usleep((useconds_t)((MINIMUM_REFRESH - elapsed) * 1000000));

    CAN_read();
    UEI_read();

    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        status = (*commitem)->write();
        if (status == CommModule::Terminate) Terminate(0);

        status = (*commitem)->read();
        if (status == CommModule::Success) UpdateDisplay();
        else if (status == CommModule::Terminate) Terminate(0);

        if ((*commitem)->activeID) *((*commitem)->activeID) = (*commitem)->isActive();
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


/*********************************************************************************
 *
 *  This cleanly shuts down the application.
 *
 *********************************************************************************/
void Terminate(int flag)
{
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;
    std::list<PixelStreamItem *>::iterator psitem;

    if (AppData.DisplayClose) AppData.DisplayClose();

    ui_terminate();
    CAN_term();

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
    varlist_term();
    exit(flag);
}

static std::vector<std::string> parseUserPath(void)
{
    std::vector<std::string> retvec;

    char *mypath = getenv("PATH");

    if (!mypath) return retvec;

    std::string userpath = mypath;

    if (userpath.empty()) return retvec;

    size_t start = 0;
    size_t end = userpath.find(':');

    while (end != std::string::npos)
    {
        retvec.push_back(userpath.substr(start, end-start));
        start = end + 1;
        end = userpath.find(':', start);
    }
    retvec.push_back(userpath.substr(start));

    return retvec;
}

static char *findExecutablePath(std::string inpath)
{
    PathInfo *pinfo;

    if (inpath.find('/') != std::string::npos)
    {
        pinfo = new PathInfo(inpath);
        if (pinfo->isExecutableFile()) return pinfo->getDirectory();
        else
        {
            delete pinfo;
            return 0x0;
        }
    }

    std::vector<std::string> upath = parseUserPath();
    std::vector<std::string>::iterator item;
    for (item = upath.begin(); item != upath.end(); item++)
    {
        pinfo = new PathInfo(*item + '/' + inpath);
        if (pinfo->isExecutableFile()) return pinfo->getDirectory();
        delete pinfo;
    }

    return 0x0;
}

static void setenvUsingScript(const char *myenv, const char *myscript, const char *myarg)
{
    char *cmd;
    asprintf(&cmd, "%s %s", myscript, myarg);

    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        free(cmd);
        return;
    }

    size_t currentalloc = 256, bytecount = 0;
    char mychar;

    char *buffer = (char *)malloc(currentalloc);

    while (fread(&mychar, 1, 1, pipe))
    {
        bytecount++;
        if (bytecount > currentalloc)
        {
            currentalloc += 256;
            buffer = (char *)realloc((void *)buffer, currentalloc); fflush(0);
        }
        buffer[bytecount-1] = mychar;
    }

    // strip out leading and trailing white space
    size_t i, startbyte = 0, endbyte = bytecount;
    for (i=0; i<bytecount; i++)
    {
        if (isspace(buffer[i])) startbyte++;
        else break;
    }
    for (i=bytecount-1; (int)i>=0; i--)
    {
        if (isspace(buffer[i])) endbyte--;
        else break;
    }
    buffer[endbyte] = '\0';

    setenv(myenv, &buffer[startbyte], 1);

    free(buffer);
    free(cmd);
}

/* Set the dcappOSTYPE, dcappOSSPEC, dcappOBJDIR, and dcappBINDIR environment       */
/* variables in case the user needs them.  Likewise, set the following for the user */
/* if they have not been set: USER, LOGNAME, HOME, OSTYPE, MACHTYPE, and HOST.      */
static void SetDefaultEnvironment(std::string pathspec)
{
    int i;
    struct utsname minfo;
    struct passwd *pw = getpwuid(getuid());
    long hsize = sysconf(_SC_HOST_NAME_MAX)+1;
    char myhost[hsize];
    char *lc_os;
    std::string tmppath;

    char *resolvedpath = (char *)calloc(PATH_MAX, sizeof(char));

    std::string mypath = findExecutablePath(pathspec);
    tmppath = mypath + "/../../../";
    realpath(tmppath.c_str(), resolvedpath);

    AppData.dcapphome = resolvedpath;

    tmppath = mypath + "/../dcapp-config";
    realpath(tmppath.c_str(), resolvedpath);

    setenvUsingScript("dcappOSTYPE", resolvedpath, "--ostype");
    setenvUsingScript("dcappOSSPEC", resolvedpath, "--osspec");
    setenvUsingScript("dcappOBJDIR", resolvedpath, "--objdir");
    setenvUsingScript("dcappBINDIR", resolvedpath, "--bindir");

    free(resolvedpath);

    uname(&minfo);

    lc_os = strdup(minfo.sysname);
    for (i=0; i<(int)strlen(lc_os); i++) lc_os[i] = tolower(lc_os[i]);
    setenv("OSTYPE", lc_os, 0);
    free(lc_os);

    setenv("USER", pw->pw_name, 0);
    setenv("LOGNAME", pw->pw_name, 0);
    setenv("HOME", pw->pw_dir, 0);
    setenv("MACHTYPE", minfo.machine, 0);

    if (!gethostname(myhost, hsize)) setenv("HOST", myhost, 0);
}

static void ProcessArgs(int argc, char **argv)
{
    int i, count, gotargs;
    char *xdisplay = 0x0;
    char *specfile = 0x0, *args = 0x0;
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
