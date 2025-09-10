#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#endif


#include "sb.h"
#include "trick.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7000
#define BUFFER_SIZE 1024



#define TX_CMD_BUFFER_ALLOC_SIZE 512
#define TX_MAX_  256

static int socketCount = 0;

static char *resolveHostname(const char *host) {

    // Check if already a valid IP address
    struct in_addr addr4;
    struct in6_addr addr6;
    if (inet_pton(AF_INET, host, &addr4) == 1 || inet_pton(AF_INET6, host, &addr6) == 1) {
        return strdup(host);
    }

    struct addrinfo hints = {0}, *result, *entry;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &result) != 0) {
        fprintf(stderr, "Failed to resolve hostname: %s\n", host);
        return NULL;
    }

    char ipStr[INET6_ADDRSTRLEN] = {0};
    void *addrPtr = NULL;
    int family = 0;

    // default to ipv4
    for (entry = result; entry != NULL; entry = entry->ai_next) {
        if (entry->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)entry->ai_addr;
            addrPtr = &(ipv4->sin_addr);
            family = AF_INET;
            break;
        }
    }

    // fallback to ipv6
    if (!addrPtr) {
        for (entry = result; entry != NULL; entry = entry->ai_next) {
            if (entry->ai_family == AF_INET6) {
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)entry->ai_addr;
                addrPtr = &(ipv6->sin6_addr);
                family = AF_INET6;
                break;
            }
        }
    }

    char *resolved = NULL;
    if (addrPtr && inet_ntop(family, addrPtr, ipStr, sizeof(ipStr))) {
        resolved = strdup(ipStr);
    } else {
        perror("resolve hostname inet_ntop");
    }

    freeaddrinfo(result);
    return resolved;
}


void trickInitContext(TrickContext* context) {

#ifdef _WIN32
    if (!socketCount) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
#endif
    socketCount++;

    // resolve host
    context->_ip = resolveHostname(context->host);

    // connection
    context->_sockFd = -1;

    // rx
    context->_rxCmds = NULL;
    context->_rxCmdOffsets = NULL;
    context->_rxBuffer = (char*)malloc(2048);

    // rx oad
    context->_rxOads = NULL;
    context->_rxOadOffsets = NULL;
    context->_rxOadBuffer = (char*)malloc(2048);

    // tx
    context->_txCmds = NULL;
    context->_txCmdOffsets = NULL;
    context->_txBuffer = (char*)malloc(2048);
}

void trickCleanupContext(TrickContext* context) {

    socketCount--;
#ifdef _WIN32
    closesocket(context->_sockFd);
    if (!socketCount) {
        WSACleanup();
    }
#else
    close(context->_sockFd);
#endif

    sbfree(context->_rxCmds);
    sbfree(context->_rxCmdOffsets);
    sbfree(context->_rxOads);
    sbfree(context->_rxOadOffsets);
    sbfree(context->_txCmds);
    sbfree(context->_txCmdOffsets);

    free(context->_rxBuffer);
    free(context->_txBuffer);
}

int trickAddTxVariable(TrickContext* context, const char* path, const char* units, bool isString) {

    // create cmd
    if (isString) {
        sprintf(context->_txBuffer, "trick.var_set(\"%s\", \"%%s\"", path);
    } else {
        sprintf(context->_txBuffer, "trick.var_set(\"%s\", %%s", path);
    }
    if (units) {
        strcat(context->_txBuffer, ", \"");
        strcat(context->_txBuffer, units);
        strcat(context->_txBuffer, "\")\n");
    } else {
        strcat(context->_txBuffer, ")\n");
    }

    // copy
    int start = sbcount(context->_txCmds);
    sbpush(context->_txCmdOffsets, start);
    sbcat(context->_txCmds, context->_txBuffer, strlen(context->_txBuffer));
    sbpush(context->_txCmds, '\0');

    // return index
    return sbcount(context->_txCmdOffsets);
}

// leaves the calling context responsible for converting the value to a string
// not sure if this is correct....but okay for now
void trickSetVariable(TrickContext* context, int cmdIndex, const char* value) {
    sprintf(context->_txBuffer, &(context->_txCmds[context->_txCmdOffsets[cmdIndex]]), value);
    printf(context->_txBuffer);
    send(context->_sockFd, context->_txBuffer, strlen(context->_txBuffer), 0);
}

int trickAddRxVariable(TrickContext* context, const char* path, const char* units) {

    // create cmd
    sprintf(context->_rxBuffer, "trick.var_add(\"%s\"", path);
    if (units) {
        strcat(context->_rxBuffer, ", \"");
        strcat(context->_rxBuffer, units);
        strcat(context->_rxBuffer, "\")\n");
    } else {
        strcat(context->_rxBuffer, ")\n");
    }

    // copy
    int start = sbcount(context->_rxCmds);
    sbpush(context->_rxCmdOffsets, start);
    sbcat(context->_rxCmds, context->_rxBuffer, strlen(context->_rxBuffer));
    sbpush(context->_rxCmds, '\0');

    // return index
    return sbcount(context->_rxCmdOffsets);
}

int trickAddRxOadVariable(TrickContext* context, const char* path) {

    // copy
    int start = sbcount(context->_rxOads);
    sbpush(context->_rxOadOffsets, start);
    sbcat(context->_rxOads, path, strlen(path));
    sbpush(context->_rxOads, '\0');

    // return index
    return sbcount(context->_rxCmdOffsets);
}

int trickConnectToServer(TrickContext* context) {

    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    socklen_t socketAddrLen;
    int family;
    struct sockaddr *addrPtr = NULL;

    // setup ipv4 vs. v6
    if (inet_pton(AF_INET, context->_ip, &addr4.sin_addr) == 1) {
        family = AF_INET;
        addr4.sin_family = AF_INET;
        addr4.sin_port = htons(context->port);
        socketAddrLen = sizeof(addr4);
        addrPtr = (struct sockaddr *)&addr4;
    } else if (inet_pton(AF_INET6, context->_ip, &addr6.sin6_addr) == 1) {
        family = AF_INET6;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(context->port);
        socketAddrLen = sizeof(addr6);
        addrPtr = (struct sockaddr *)&addr6;
    }
    else {
        fprintf(stderr, "Invalid IP address: %s\n", context->_ip);
        return -1;
    }

    // create socket
    context->_sockFd = socket(family, SOCK_STREAM, 0);
    if (context->_sockFd < 0) {
        perror("Failed to create trick socket");
        return -1;
    }

    // TODO make this nonblocking

    // connect
    if (connect(context->_sockFd, addrPtr, socketAddrLen) < 0) {
        perror("Failed to connect to trick socket");
        close(context->_sockFd);
        return -1;
    }

    // set initial comm params
    // pause variable server
    char cmdBuffer[2048];
    strcpy(cmdBuffer, "trick.var_pause()\n");
    send(context->_sockFd, cmdBuffer, strlen(cmdBuffer), 0);
    // set sample rate
    sprintf(cmdBuffer, "trick.var_cycle(%f)\n", context->dataRate);
    send(context->_sockFd, cmdBuffer, strlen(cmdBuffer), 0);
    // write list of one-and-done rx variables to get
    // TODO cleanup
    char *oadBuffer = NULL; // stretchy buffer
    strcpy(cmdBuffer, "trick.var_send_once(\"");
    sbcat(oadBuffer, cmdBuffer, strlen(cmdBuffer));
    for (int ii = 0; ii < sbcount(context->_rxOadOffsets); ii++) {
        char *oadVar = &(context->_rxOads[context->_rxOadOffsets[ii]]);
        sbcat(oadBuffer, oadVar, strlen(oadVar));
        if (ii < sbcount(context->_rxOadOffsets) - 1) {
            sbcat(oadBuffer, ", ", strlen(", "));
        }
    }
    sprintf(cmdBuffer, "\", %d)\n", sbcount(context->_rxOadOffsets));
    sbcat(oadBuffer, cmdBuffer, strlen(cmdBuffer));
    send(context->_sockFd, oadBuffer, sbcount(oadBuffer), 0);
    // write list of variables to listen to
    for (int ii = 0; ii < sbcount(context->_rxCmdOffsets); ii++) {
        char *varAddCmd = &(context->_rxCmds[context->_rxCmdOffsets[ii]]);
        send(context->_sockFd, varAddCmd, strlen(varAddCmd), 0);
    }
    // unpause
    strcpy(cmdBuffer, "trick.var_unpause()\n");
    send(context->_sockFd, cmdBuffer, strlen(cmdBuffer), 0);

    // connection now active
    context->isAlive = true;
    return 0;
}

int trickReceive(TrickContext *context) {
    int bytes = recv(context->_sockFd, context->_rxBuffer, 2047, 0);
    if (bytes > 0) {
        context->_rxBuffer[bytes] = '\0';
        printf("Update: %s", context->_rxBuffer);
    }
}

int main() {

    TrickContext context;
    context.host     = strdup("localhost");
    context.port     = 7000;
    context.dataRate = .2;
    trickInitContext(&context);

    trickAddRxVariable(&context, "dyn.cannon.init_speed", NULL);
    trickAddRxVariable(&context, "dyn.cannon.time", NULL);
    trickAddRxVariable(&context, "dyn.cannon.init_fspeed", NULL);
    trickAddRxVariable(&context, "dyn.cannon.init_speed", NULL);

    trickAddRxOadVariable(&context, "dyn.cannon.time");

    trickAddTxVariable(&context, "dyn.cannon.init_speed", NULL, false);

    for (int ii = 0; ii < sbcount(context._rxCmdOffsets); ii++) {
        printf(&(context._rxCmds[context._rxCmdOffsets[ii]]));
    }

    printf(context._ip);

    trickConnectToServer(&context);
    if (!context.isAlive) {
        perror("Failed to connect to trick server");
        return 1;
    }


    // Simulate reading updates and sending new values
    // for (int i = 0; i < 10; i++) {
    //     read_updates();
    //     send_update("dyn.cannon.g", 1000.0 + i * 10);
    //     sleep(1);
    // }
    while (1) {
        trickReceive(&context);
        trickSetVariable(&context, 0, "10");
    }

    trickCleanupContext(&context);
    return 0;
}

// int sockfd;

// // Connect to the Trick Variable Server
// int connect_to_server() {
//     struct sockaddr_in server_addr;

//     sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd < 0) {
//         perror("Socket creation failed");
//         return -1;
//     }

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(SERVER_PORT);
//     inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

//     if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
//         perror("Connection failed");
//         return -1;
//     }

//     return 0;
// }

// // Register a variable to monitor
// void add_variable(const char *var_name) {
//     char cmd[BUFFER_SIZE];
//     snprintf(cmd, sizeof(cmd), "trick.var_add(\"%s\")\n", var_name);
//     send(sockfd, cmd, strlen(cmd), 0);
// }

// // Read variable updates
// void read_updates() {
//     char buffer[BUFFER_SIZE];
//     int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
//     if (bytes > 0) {
//         buffer[bytes] = '\0';
//         printf("Update: %s", buffer);
//     }
// }

// // Send updated value to sim
// void send_update(const char *var_name, double value) {
//     char cmd[BUFFER_SIZE];
//     snprintf(cmd, sizeof(cmd), "trick.var_set(\"%s\", %lf)\n", var_name, value);
//     send(sockfd, cmd, strlen(cmd), 0);
// }

// int main() {
//     if (connect_to_server() != 0) {
//         return 1;
//     }

//     // Example usage
//     add_variable("dyn.cannon.init_speed");
//     add_variable("dyn.cannon.time");
//     add_variable("dyn.cannon.init_fspeed");
//     add_variable("dyn.cannon.init_speed");

//     // Simulate reading updates and sending new values
//     for (int i = 0; i < 10; i++) {
//         read_updates();
//         send_update("dyn.cannon.g", 1000.0 + i * 10);
//         sleep(1);
//     }

//     close(sockfd);
//     return 0;
// }
