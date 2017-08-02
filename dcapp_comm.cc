//#include <string.h>
#include <assert.h>

#include "dcapp_comm.hh"
#include "packages/imgload/imgload.hh"
#include "trick/tc_proto.h"
#include "nodes.hh"


extern appdata AppData;

namespace DCAPP_Server {
	
	size_t ReadArithmeticType( char *buffA, uint32_t *outA )
	{
		const size_t sz = sizeof(uint32_t);
		
		union {
			uint32_t val;
			char ch[sz];
		} u = {0};
		
		for (size_t i = 0; i < sz; ++i)
			u.ch[i] = buffA[i];
		
		*outA = u.val;
		return sz;
	}
	
	//////
	// ReadArithmeticType
	//
	// Deserialize a single value of an arithmetic type from a buffer of
	// characters.
	template <typename T>
	size_t ReadArithmeticType( const std::vector<char> &buff, T *out)
	{
		const size_t sz = sizeof(T);
		if (buff.size() < sz)
			throw std::range_error("out of reading room");
		union {
			T val;
			char ch[sz];
		} u = {0};
		
		for (size_t i = 0; i < sz; ++i)
			u.ch[i] = buff[i];

		*out = u.val;
		return sz;
	}

	////
	// WriteArithmeticTypeImpl
	//
	// Serialize a single value of an arithmetic type to a buffer of
	// (mutable) characters.
	//
	//
	template <typename T>
	size_t WriteArithmeticType( std::vector<char> &buff, T val)
	{
		const size_t sz = sizeof(T);
		
		union {
			T val;
			char ch[sz];
		} u = { val };

		for (size_t i = 0; i < sz; ++i)
			buff.push_back( u.ch[i] );

		return sz;
	}
	
	template <typename T>
	size_t WriteArithmeticType( char buff[4], T val)
	{
		const size_t sz = sizeof(T);
		
		union {
			T val;
			char ch[sz];
		} u = { val };
		
		for (size_t i = 0; i < sz; ++i)
			buff[i] = u.ch[i];
		
		return sz;
	}
	
	size_t WriteString( std::vector<char> &buff, const char *stringA )
	{
		const size_t sz = strlen( stringA );

		for (size_t iL = 0; iL < sz; ++iL )
			buff.push_back( stringA[iL] );
		
		return sz;
	}
	
	size_t WriteString( std::vector<char> &buff, const std::vector< unsigned char > &stringA )
	{
		const size_t sz = stringA.size();
		for (size_t iL = 0; iL < sz; ++iL )
			buff.push_back( stringA[iL] );
		
		return sz;
	}
	
	const static std::map< DCA_Comm::Commands, uint32_t >	CommandMap = {
		{ DCA_Comm::Commands::Invalid, 0},
		{ DCA_Comm::Commands::XMLRequest, 1},
		{ DCA_Comm::Commands::XMLData, 2},
		{ DCA_Comm::Commands::TextureCount, 3 },
		{ DCA_Comm::Commands::FileData, 4 },
		{ DCA_Comm::Commands::FetchTextureMap, 5 },
		{ DCA_Comm::Commands::FetchFont, 6 }

	};
	
	uint32_t EncodeCommand( DCA_Comm::Commands commandA )
	{
		if( CommandMap.find( commandA ) == CommandMap.end() )
			throw std::runtime_error( "Bad Command" );
		
		return CommandMap.find( commandA )->second;
	}
	
	DCA_Comm::Commands DecodeCommand( uint32_t valueA )
	{
		for( const auto cmdValue : CommandMap )
		{
			if( cmdValue.second == valueA )
				return cmdValue.first;
		}
		
		return DCA_Comm::Commands::Invalid;
	}
};

DCA_Comm::DCA_Comm( void )
{
	memset( &m_Connection, '\0', sizeof( TCDevice ) );
	
	m_IsConnected = false;
}

DCA_Comm::~DCA_Comm( void )
{
	if( tc_isValid( &m_Connection ) )
	{
		tc_disconnect( &m_Connection );
	}
	
}
void DCA_Comm::Close( void )
{
	tc_disconnect( &m_Connection );
}

void DCA_Comm::SetHostName( const char *nameA )
{
	if( m_Connection.hostname != nullptr )
		free( m_Connection.hostname );
}

std::shared_ptr< DCA_Comm > DCA_Comm::Listen( void )
{
	if( tc_listen( &m_Connection ) )
	{
		auto pCommL = std::make_shared<DCA_Comm>();
		
		auto statusL = tc_accept( &m_Connection, &pCommL->m_Connection );
		
		if( statusL != TC_SUCCESS )
		{
			pCommL = nullptr;

			return( nullptr );
		}
		
		tc_blockio( &pCommL->m_Connection, TC_COMM_ALL_OR_NOTHING );
		
		return( pCommL );
	}
	return nullptr;
}

bool DCA_Comm::Connect( void )
{
	for( size_t attempts = 0; attempts < 5; attempts++ )
	{
		if( m_IsConnected )
			return true;
		
		if( tc_connect( &m_Connection ) == TC_SUCCESS )
			m_IsConnected = true;
		
	}
	return false;
}

bool DCA_Comm::isConnected( void ) const
{
	return m_IsConnected;
}


bool DCA_Comm::SendCommand( DCA_Comm::Commands commandA )
{
	if( tc_isValid( &m_Connection ) == false )
		return false;
	
	char sizeL[4];
	
	uint32_t dataSizeL = static_cast<uint32_t>( sizeof( uint32_t ) );
	DCAPP_Server::WriteArithmeticType( sizeL, dataSizeL ); // send size of data
	
	std::vector<char> dataL;
	DCAPP_Server::WriteArithmeticType( dataL, DCAPP_Server::EncodeCommand( commandA ) ); // send size of data

	assert( dataL.size() == dataSizeL );
	
	auto byteCountL = static_cast<int>(sizeof( sizeL ));
	
	while( tc_write( &m_Connection, sizeL, byteCountL ) != byteCountL )
	{
	}

	size_t so_far = 0;
	do {
		so_far += tc_write( &m_Connection, dataL.data() + so_far, static_cast<int>( dataSizeL - so_far ));
	} while( so_far < dataSizeL );
	
	return true;
}

std::vector< char > DCA_Comm::ReadDataPacket( void )
{
	std::vector< char >	dataL;
	if( tc_isValid( &m_Connection ) == false )
		return dataL;

	bool doneL = false;
	while( doneL == false )
	{
		auto pendL = tc_pending( &m_Connection );
		
		if( pendL > 0 )
		{
			char sizeL[4];
			tc_read( &m_Connection, sizeL, sizeof(sizeL) );
			
			uint32_t commandSizeL;
			DCAPP_Server::ReadArithmeticType( sizeL, &commandSizeL );
			
			if( commandSizeL > 0 )
			{
				std::vector<char> dataL;
				
				dataL.resize( commandSizeL );
				
				auto nreadL = 0;
				auto tryL    = 0;
				while( nreadL < commandSizeL )
				{
					auto cntL = tc_read( &m_Connection, &dataL[nreadL], commandSizeL-nreadL );
					
					if( cntL > 0 )
					{
						nreadL += cntL;
					}
					else
					{
						if( tryL < 5 )
							tryL++;
						else
							doneL = true;
					}
				}
				
				return dataL;
			}
		}
		else
			doneL = true;
	}
	
	return dataL;
}

DCA_Comm::Commands DCA_Comm::ReadCommand( void )
{
	auto dataL = ReadDataPacket();
	
	if( dataL.empty() == false )
	{
		uint32_t valueL;
		DCAPP_Server::ReadArithmeticType( dataL, &valueL );
		return DCAPP_Server::DecodeCommand( valueL );
	}
	
	return DCA_Comm::Commands::Invalid;
}

int DCA_Comm::Read( uint32_t &valueA )
{
	std::vector< char > dataL;
 
	while( dataL.empty() )
	{
		dataL = ReadDataPacket();
		
		if( dataL.empty() == false )
		{
			DCAPP_Server::ReadArithmeticType( dataL, &valueA);
		
			return sizeof( valueA );
		}
	}
	
	return -1;
}

bool DCA_Comm::Read( std::string &outputA )
{
	std::vector< char > dataL;
 
	while( dataL.empty() )
	{
		dataL = ReadDataPacket();
		
		if( dataL.empty() == false )
			outputA = std::string( dataL.begin(), dataL.end() );
		
//		return true;
	}
	
	return true;
}

int DCA_Comm::Read( std::vector< unsigned char > &outputA )
{
	std::vector< char > dataL;
 
	while( dataL.empty() )
	{
		dataL = ReadDataPacket();
		
		if( dataL.empty() == false )
			outputA = std::vector<unsigned char>( dataL.begin(), dataL.end() );
	}
	
	return static_cast<int>(outputA.size());
}


/*-------------------------------------------------------------------------*/

void DCA_Comm::Send( size_t valueA )
{
	char sizeL[4];
	const uint32_t dataSizeL = 4;
	
	auto byteCountL = static_cast<int>(sizeof( sizeL ));
	
	DCAPP_Server::WriteArithmeticType( sizeL, dataSizeL ); // send size of data
	while( tc_write( &m_Connection, sizeL, byteCountL ) != byteCountL )
	{
	}
	
	DCAPP_Server::WriteArithmeticType( sizeL, valueA );

	while( tc_write( &m_Connection, sizeL, byteCountL ) != byteCountL )
	{
	}
}

void DCA_Comm::Send( const std::vector< char > &pDataA )
{
	char sizeL[4];
	
	uint32_t dataSizeL = static_cast<uint32_t>( pDataA.size() );
	
	DCAPP_Server::WriteArithmeticType( sizeL, dataSizeL ); // send size of data
	
	auto byteCountL = static_cast<int>(sizeof( sizeL ));
	
	while( tc_write( &m_Connection, sizeL, byteCountL ) != byteCountL )
	{
	}
	
	// if tc_write was const correct this would not be needed.
	auto pLocalL = (char*)(const_cast<char *>( pDataA.data() ));
	
	size_t so_far = 0;
	do {
		so_far += tc_write( &m_Connection, pLocalL + so_far, static_cast<int>( dataSizeL - so_far ));
	} while( so_far < dataSizeL );
}

bool DCA_Comm::ReadFile( void )
{
	//  CHECK FOR VALID CONNECTION
	if( tc_isValid( &m_Connection ) == false )
		return false;
	
	std::string texNameL;
	Read( texNameL );
	
	std::string fileDataL;
	
	Read( fileDataL );
	
	std::string pathL = getBasePath();
	
	pathL += "/" + texNameL;
	
	SaveFileData( pathL, fileDataL );
	
	return true;
}

bool DCA_Comm::SendFile( const std::string &fileNameA, const std::string &fullNameA )
{
	//  CHECK FOR VALID CONNECTION
	if( tc_isValid( &m_Connection ) == false )
		return false;

	std::string testDataL = LoadFileAsOne( fullNameA );
	if( testDataL.empty() )
		return false;
	
	SendCommand( DCA_Comm::Commands::FileData );
	Send( fileNameA );
	Send( testDataL );
	
	return true;
}

void DCA_Comm::Send( const char *pDataA )
{
	char sizeL[4];

	uint32_t dataSizeL = static_cast<uint32_t>(strlen( pDataA ));
	
	DCAPP_Server::WriteArithmeticType( sizeL, dataSizeL ); // send size of data
	
	while( tc_write( &m_Connection, sizeL, sizeof(sizeL) ) != sizeof(sizeL) )
	{
	}

	// if tc_write was const correct this would not be needed.
	auto pLocalL = const_cast<char *>( pDataA );
	
	size_t so_far = 0;
	do {
		so_far += tc_write( &m_Connection, pLocalL + so_far, static_cast<int>( dataSizeL - so_far ));
	} while( so_far < dataSizeL );
}

void DCA_Comm::Send( const std::string &stringA )
{
	char sizeL[4];
	
	uint32_t dataSizeL = static_cast<uint32_t>( stringA.size() );
	
	auto byteCountL = static_cast<int>(sizeof( sizeL ));
	
	DCAPP_Server::WriteArithmeticType( sizeL, dataSizeL ); // send size of data
	
	while( tc_write( &m_Connection, sizeL, byteCountL) != byteCountL )
	{
	}
	
	// if tc_write was const correct this would not be needed.
	auto pLocalL = const_cast<char *>( stringA.c_str() );
	
	size_t so_far = 0;
	do {
		so_far += tc_write( &m_Connection, pLocalL + so_far, static_cast<int>( dataSizeL - so_far ));
	} while( so_far < dataSizeL );
}

/*-------------------------------------------------------------------------*/
std::shared_ptr< DCA_Comm > Open( DCA_Comm::Type typeA, const char *hostNameA, int portA )
{
	switch( typeA )
	{
		case DCA_Comm::Type::Server:
		{
			auto ptrL = std::make_shared<DCA_Comm>();
 		
			ptrL->m_Connection.port = portA;
			tc_init( &ptrL->m_Connection );
			
			ptrL->m_Connection.disable_handshaking = TC_COMM_TRUE;
			ptrL->m_Connection.disable_handshaking = TC_COMM_FALSE;
//			ptrL->m_Connection.blockio_type = TC_COMM_NOBLOCKIO;
			
			return ptrL;
		}
			break;
			
		case DCA_Comm::Type::Client:
		{
			auto ptrL = std::make_shared<DCA_Comm>();
			
			ptrL->m_Connection.port = portA;
			ptrL->m_Connection.disable_handshaking = TC_COMM_TRUE;
			ptrL->m_Connection.disable_handshaking = TC_COMM_FALSE;
//			ptrL->m_Connection.blockio_type = TC_COMM_NOBLOCKIO;

			if( hostNameA )
			{
				ptrL->m_Connection.hostname = strdup( hostNameA );
			}
			tc_error(&ptrL->m_Connection, 0);

			
			return ptrL;
			break;
		}
	}
	
	return( nullptr );
}


namespace DCAPP_Server {

	
}
