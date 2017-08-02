#ifndef _DCAPP_COMM_H_
#define _DCAPP_COMM_H_
#include <vector>
#include <memory>

#include "trick/tc.h"


class DCA_Comm
{
public:
		DCA_Comm( void );
	   ~DCA_Comm( void );
	
	enum Commands	{ Invalid, XMLRequest, XMLData, TextureCount, FileData, FetchTextureMap, FetchFont };
	
	std::shared_ptr< DCA_Comm >	Listen	( void );

	int			Read		( uint32_t &valueA );
	bool		Read		( std::string &stringA );
	int			Read		( std::vector< unsigned char > &outputA );
	
	bool		Connect		( void );
	void		Close		( void );
	
	bool					SendCommand		( DCA_Comm::Commands commandA );
	DCA_Comm::Commands		ReadCommand		( void );
	
	void	Send		( const char *pDataA );
	void	Send		( const std::string &stringA );
	void	Send		( const std::vector< char > &pDataA );
	void	Send		( size_t valueA );

	bool	SendFile	( const std::string &fileNameA, const std::string &fullNameA );
	bool	ReadFile	( void );
	
	void	SetHostName	( const char *nameA );

	bool	isConnected	( void ) const;

private:
	std::vector< char >	ReadDataPacket	( void );
	
public:
	enum Type { Server, Client };

public:
	TCDevice	m_Connection;
	bool		m_IsConnected;
};

std::shared_ptr< DCA_Comm >	Open	( DCA_Comm::Type typeA, const char *hostNameA, int portA );

#endif // _DCAPP_COMM_H_
