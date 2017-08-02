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
#include <iostream>
#include <fstream>

#include "basicutils/msg.hh"
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "packages/imgload/imgload.hh"
#include "varlist.hh"
#include "can/CAN.hh"
#include "uei/UEI.hh"
#include "nodes.hh"
#include "string_utils.hh"
#include "xml_stringsub.hh"
#include "osenv/osenv.hh"
#include "dcapp_comm.hh"
#include "string_utils.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

extern void mainloop(void);
extern void UpdateDisplay(void);
extern void SetNeedsRedraw(void);
extern void CheckMouseBounce(void);
extern void ui_init(char *);
extern void ui_terminate(void);
extern int ParseXMLFile(const char *);
int ParseXMLString( const char *pDataA );
extern void DisplayPreInitStub(void *(*)(const char *));
extern void DisplayInitStub(void);
extern void DisplayLogicStub(void);
extern void DisplayCloseStub(void);

void Terminate(int);

static void SetDefaultEnvironment(void);
static void ProcessArgs(int, char **);

appdata AppData;

static std::vector< std::shared_ptr< DCA_Comm > >	ActiveComm;

void InitializeData( int argc, char **argv )
{
    Message::setLabel(basename(argv[0]));
	
	AppData.isServer	= false;
	AppData.isClient	= false;
    AppData.master_timer = new Timer;
    AppData.last_update = new Timer;
    
    AppData.DisplayPreInit = &DisplayPreInitStub;
    AppData.DisplayInit = &DisplayInitStub;
    AppData.DisplayLogic = &DisplayLogicStub;
    AppData.DisplayClose = &DisplayCloseStub;
	
	AppData.dcappServerPortNumber	 = "8042";

    SetDefaultEnvironment();
	ProcessArgs(argc, argv);
#ifndef IOS_BUILD
	
	AppData.isServer = true;
	
	if( AppData.isServer )
	{
		std::cout << "server port number" << AppData.dcappServerPortNumber << std::endl;
		AppData.m_Server = Open( DCA_Comm::Type::Server, "", StrToInt(AppData.dcappServerPortNumber.c_str(), 8042 ) );
	}
#else
	AppData.isClient = true;
	AppData.xml_FileName = "";
#endif
	
	if( AppData.isServer || AppData.xml_FileName.empty() == false )
	{
		if( ParseXMLFile( AppData.xml_FileName.c_str() ) )
			Terminate(-1);
	}
	else if( AppData.isClient && AppData.xml_FileData.empty() == false )
	{
		ParseXMLString( AppData.xml_FileData.c_str() );
	}
	
    AppData.DisplayPreInit(get_pointer);
    AppData.DisplayInit();
	
    UpdateDisplay();
	
	mainloop();
}

/*********************************************************************************
 *
 *  The main routine. Calls setup, handlers, and event loop.
 *
 *********************************************************************************/
#ifndef IOS_BUILD
int main(int argc, char **argv)
{
    // Set up signal handlers
    signal(SIGINT, Terminate);
    signal(SIGTERM, Terminate);
    signal(SIGPIPE, SIG_IGN);

    InitializeData( argc, argv );

    return(0);
}
#endif

bool RequestFile( DCA_Comm::Commands commandA, const std::string &fileNameA )
{
	if( fileNameA.empty() )
		return false;
	
	AppData.m_Client->Send( commandA );
	AppData.m_Client->Send( fileNameA );
	
	bool doneL = false;
	while( doneL == false )
	{
		DCA_Comm::Commands commandL = AppData.m_Client->ReadCommand();
		
		if( commandL == DCA_Comm::Commands::FileData )
		{
			AppData.m_Client->ReadFile();
			return true;
		}
	}
	
	return false;
}

void ReadXMLDataFromServer( const char *hostNameA, const char *portNumberA )
{
	std::cout << "host " << hostNameA << " port " << portNumberA << std::endl;
	AppData.m_Client = Open( DCA_Comm::Type::Client, hostNameA, StrToInt(portNumberA, 8042) );
	
	size_t tryCountL = 0;
	
	while( tryCountL < 5 && AppData.m_Client->Connect() == false )
	{
		tryCountL++;
	}
	
	if( tryCountL >= 5 )
		return;
	
	AppData.m_Client->SendCommand( DCA_Comm::Commands::XMLRequest );

	bool doneL = false;
	while( !doneL )
	{
		DCA_Comm::Commands commandL = AppData.m_Client->ReadCommand();

		if( commandL == DCA_Comm::Commands::XMLData )
		{
			AppData.m_Client->Read( AppData.xml_FileData );
			doneL = true;
		}
	}
	
	char **args = new char *[2];
	
	args[0] = strdup( "DCApp_Mobile" );
	args[1] = strdup( "dummy" );
	
	InitializeData( 2, args );

	free( args[1] );
	free( args[0] );
	
	free( args );
}

/*********************************************************************************
 *
 *  This is where the application sits until it gets an event.
 *
 *********************************************************************************/
void Idle(void)
{
    int status;
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;

    CAN_read();
    UEI_read();
	
#ifndef IOS_BUILD
	if( AppData.isServer )
	{
		auto newConnection = AppData.m_Server->Listen();
		if( newConnection != nullptr )
		{
			ActiveComm.push_back( newConnection );
		}
		
		for( auto commL : ActiveComm )
		{
			DCA_Comm::Commands commandL = commL->ReadCommand();
			
			if( commandL == DCA_Comm::Commands::XMLRequest )
			{
				std::ifstream	inputL;
		
				inputL.open( AppData.xmlTmpFileName );
		
				std::string	fileDataL;
		
				std::string stringL;
		
				while( getline( inputL, stringL ) )
				{
					fileDataL += stringL;
					fileDataL += "\n";
				}
				inputL.close();

				commL->SendCommand( DCA_Comm::Commands::XMLData );
				commL->Send( fileDataL.c_str() );
				std::cout << "Sending XML with a size of " << fileDataL.size() << std::endl;
			}
			else if( commandL == DCA_Comm::Commands::FetchTextureMap )
			{
				std::string fileNameL;
				commL->Read( fileNameL );
				std::cout << "Sending texture map " << fileNameL << std::endl;
				
				auto textureL = getTextureInfo( fileNameL );
				commL->SendFile( textureL->m_Filename, textureL->m_FullPath );
			}
			else if( commandL == DCA_Comm::Commands::FetchFont )
			{
				std::string fileNameL;
				commL->Read( fileNameL );
				std::cout << "Sending font " << fileNameL << std::endl;
				
				auto fullPathL = getFullPath( fileNameL );
				fullPathL = getFontPath( fileNameL );
				commL->SendFile( fileNameL, fullPathL );
			}
		}
	}
	else if( AppData.isClient )
	{
	}
#endif
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

    varlist_term();
    exit(flag);
}

/* Make sure that the following environment variables are set in case   */
/* the user needs them: USER, LOGNAME, HOME, OSTYPE, MACHTYPE, and HOST */
static void SetDefaultEnvironment(void)
{
#ifndef IOS_BUILD
    int i;
    struct utsname minfo;
    struct passwd *pw = getpwuid(getuid());
    long hsize = sysconf(_SC_HOST_NAME_MAX)+1;
    char myhost[hsize];
    char *lc_os;

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
#endif
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
            if (count == 2) processArgument(key, value);
            if (count >= 1) strptr += strlen(key);
            if (count == 2) strptr += strlen(value) + 1;
            while (strptr < (args + strlen(args)) && *strptr == ' ') strptr++;
        }

        free(key);
        free(value);
    }
	
	AppData.xml_FileName = specfile;
	
    storeArgs(specfile, args);

    TIDY(specfile);
    TIDY(args);
}
