#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include "msg.hh"

#define MAX_SEND_SIZE 4096
#define BUF_SIZE_INCREMENT 256
#define END_OF_MSG_CHAR 0x04

static int edgercs_active = 0;
static struct addrinfo *server_addr_info;

static int read_rcs_message(int, char **);


int EDGE_rcs_init(char *hoststr, char *portstr)
{
    char *edgehost, *edgeport;
    struct addrinfo hints;
    int result;

    if (hoststr == NULL)
        edgehost = strdup("localhost");
    else
        edgehost = strdup(hoststr);

    if (portstr == NULL)
        edgeport = strdup("5451");
    else
        edgeport = strdup(portstr);

    /* Set up the connection hints */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    /* resolve into a list of addresses */
    result = getaddrinfo(edgehost, edgeport, &hints, &server_addr_info);
    if (result)
    {
        error_msg("Error in getaddrinfo for host %s: %s", edgehost, gai_strerror(result));
        return -1;
    }

    edgercs_active = 1;

    return 0;
}


void EDGE_rcs_term(void)
{
    freeaddrinfo(server_addr_info);
}


/* Send a single command. */
int send_doug_command(const char *doug_command, char **command_result, char **rcs_version)
{
    struct addrinfo *addr;
    size_t len;
    int command_socket = -1;
    struct timeval timeout;

    if (!edgercs_active) return 0;

    len = strlen(doug_command);
    if (len <= 0) return 0;
    else if (len >= MAX_SEND_SIZE)
    {
        debug_msg("Command too long.  Ignoring...");
        return 1;
    }

    /* Loop through available addresses until we get a socket connection */
    for (addr = server_addr_info; addr != NULL && command_socket < 0; addr = addr->ai_next)
    {
        /* Try creating the socket */
        command_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (command_socket >= 0)
        {
            if (connect(command_socket, (struct sockaddr *)addr->ai_addr, addr->ai_addrlen) < 0)
            {
                close(command_socket);
                command_socket = -1;
            }
        }
    }

    if (command_socket < 0)
    {
        debug_msg("Couldn't create socket");
        return 1;
    }

    // Try using a receive timeout, in case the EDGE RCS hangs, we don't wait forever.
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
    setsockopt(command_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));

    /* Read the server version off the socket */
    if (read_rcs_message(command_socket, rcs_version) == -1)
    {
        close(command_socket);
        return 1;
    }

    if (write(command_socket, doug_command, len) != (int)len)
    {
        debug_msg("Partial/failed write");
        close(command_socket);
        return 1;
    }

    /* Signal the server that we are done sending */
    shutdown(command_socket, SHUT_WR);

    /* Now read exec result of command coming back... this only
       works with the updated rcs server done after feb 17, 2010 */
    if (read_rcs_message(command_socket, command_result) == -1)
    {
        close(command_socket);
        return 1;
    }

    close(command_socket);
    return 0;
}


/* Reads a single message from the remote commanding server.. */
static int read_rcs_message(int socket, char **rcs_response_string)
{
    char incoming_char;
    int more_to_read = 1, nread = 0, buf_size = BUF_SIZE_INCREMENT;

    if (rcs_response_string)
    {
        *rcs_response_string = (char *)realloc(*rcs_response_string, buf_size);
        if (!(*rcs_response_string))
        {
            error_msg("Memory allocation error");
            return -1;
        }
    }

    /* Read off one char at a time until we hit END_OF_MSG_CHAR */
    while (more_to_read)
    {
        if (read(socket, &incoming_char, 1) != 1)
        {
            debug_msg("Failed to read character from socket");
            return -1;
        }

        if (incoming_char == END_OF_MSG_CHAR) more_to_read = 0;
        else
        {
            if (rcs_response_string)
            {
                // if the result container is too small, grow it
                if (nread + 2 > buf_size)
                {
                    buf_size += BUF_SIZE_INCREMENT;
                    *rcs_response_string = (char *)realloc(*rcs_response_string, buf_size);
                    if (!(*rcs_response_string))
                    {
                        error_msg("Memory allocation error");
                        return -1;
                    }
                }
    
                (*rcs_response_string)[nread] = incoming_char;
            }

            nread++;
        }
    }

    if (rcs_response_string) (*rcs_response_string)[nread] = '\0';

    return nread;
}
