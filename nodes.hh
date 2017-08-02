#ifndef _NODES_HH_
#define _NODES_HH_

#include <list>
#include "basicutils/timer.hh"
#include "primitives/primitives.hh"
#include "dc.hh"
#include "animation.hh"
#include "comm.hh"
#include "psi.hh"
#include "dcapp_comm.hh"

typedef struct
{
	bool	isServer;
	bool	isClient;
    float force_update;
    Timer *last_update;
    Timer *master_timer;
    std::list<Animation *> animators;
    std::list<CommModule *> commlist;
    std::list<PixelStreamItem *> pixelstreams;
    std::list<dcObject *> events;
    std::list<dcObject *> mouseheld;
    std::vector<float> vertices;
    dcWindow *toplevel;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
    int *canbus_inhibited;
	
	std::string	xmlTmpFileName;
#ifdef IOS_BUILD
	std::string	dougHostName;
	std::string dougPortNumber;
	std::string pixelStreamHostName;
	std::string pixelStreamPortNumber;
	std::string trickHostName;
	std::string trickPortNumber;
	std::string documentsFolder;
	
#endif
	bool		forceDownload;
	std::string dcappServerPortNumber;
	std::shared_ptr< DCA_Comm >	m_Server;
	std::shared_ptr< DCA_Comm >	m_Client;
	
	std::string	xml_FileName;
	std::string	xml_FileData; // only one of these two are valid
} appdata;

#endif
