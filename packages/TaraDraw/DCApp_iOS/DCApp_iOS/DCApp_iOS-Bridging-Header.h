//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

void Bridged_Initialize( const char *hostNameA, const char *portNumberA );
void Bridged_Input( float xA, float yA, int stateA );
void Bridged_TimerUpdate();
void Bridged_Draw();
void Bridged_SetDougHostAndPort( const char *hostNameA, const char *portNumberA );
void Bridged_SetPixelStreamHostAndPort( const char *hostNameA, const char *portNumberA );
void Bridged_SetTrickHostAndPort( const char *hostNameA, const char *portNumberA );
void Bridged_SetdcAppHostAndPort( const char *hostNameA, const char *portNumberA );
void Bridged_SetForceDownload( int valueA );
