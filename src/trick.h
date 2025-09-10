#ifndef _DC_TRICK_
#define _DC_TRICK_

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7000
#define BUFFER_SIZE 1024

#include <stdbool.h>


// dcapp side
// set -> set trick side variable to value
// needs
//  0) trick context (which trick server)
//  1) trick variable path
//  2) value (DcValue)
//  3) type (for quotes)
//  ** DO NOT NEED UNITS **
// but really, all this does is set a DcValue to a value
//  how to propagate to trick??

#ifdef _WIN32
typedef unsigned long long SOCKET;
#endif

enum TRICK_VAR_TYPE {
    TRICK_VAR_TYPE_UNDEFINED,
    TRICK_VAR_TYPE_STRING,
    TRICK_VAR_TYPE_INTEGER,
    TRICK_VAR_TYPE_FLOAT,
    TRICK_VAR_TYPE_BOOLEAN
};

typedef struct _TrickContext {

    // public params
    char* host;       // hostname/ip of variable server
    int   port;       // port of variable server
    bool  isAlive;    // whether connection is alive
    float dataRate;   // data rate of variable server (1/Hz)
    bool  hasNewData; // whether data is new or not

    // private params
    char* _ip;     // IP of host (resolved internally)

    // socket
#ifdef _WIN32
    SOCKET _sockFd;
#else
    int _sockFd;
#endif

    char *_rxCmds;       // stretchy buffer of vars
    int  *_rxCmdOffsets; // stretchy buffer of var offsets
    char *_rxBuffer;     // internal buffer used for misc. ops

    char *_rxOads;       // stretchy buffer of one-and-done rx vars
    int  *_rxOadOffsets; // stretchy buffer of one-and-done rx var offsets
    char *_rxOadBuffer;  // internal buffer used for misc. ops

    char *_txCmds;       // stretchy buffer of var commands
    int  *_txCmdOffsets; // stretchy buffer of var command offsets
    char *_txBuffer;     // internal buffer used for misc. ops
} TrickContext;

// functions
// setVariable  (string, float, int, etc.)
// registerVariable
// checkVariableExists
// getVariableValue (string, float, int, etc.)
// connectToServer
//      * checks which variables actually exist
//      * removes the ones that don't
// close

#endif
