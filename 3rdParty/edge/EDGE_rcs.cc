#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "basicutils/msg.hh"
#include "EDGE_rcs.hh"

#define MAX_SEND_SIZE 4096
#define BUF_SIZE_INCREMENT 256
#define END_OF_MSG_CHAR 0x04

EdgeRcsComm::EdgeRcsComm() : edgercs_active(false), server_addr_info(0x0) { }

EdgeRcsComm::~EdgeRcsComm()
{
    freeaddrinfo(this->server_addr_info);
}

int EdgeRcsComm::initialize(std::string &edgehost, std::string &edgeport)
{
    if (edgehost.empty()) edgehost = "localhost";
    if (edgeport.empty()) edgeport = "5451";

    /* Set up the connection hints */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    /* resolve into a list of addresses */
    int result = getaddrinfo(edgehost.c_str(), edgeport.c_str(), &hints, &(this->server_addr_info));
    if (result)
    {
        error_msg("Error in getaddrinfo for port " << edgeport << " on host " << edgehost << ": " << gai_strerror(result));
        return -1;
    }

    this->edgercs_active = true;

    return 0;
}

/* Send a single command. */
int EdgeRcsComm::send_doug_command(std::string &doug_command, char **command_result, char **rcs_version)
{
    struct addrinfo *addr;
    size_t len;
    int valopt = 0, command_socket = -1;
    struct timeval timeout;
    long arg; 
    fd_set myset; 
    static socklen_t valopt_len = sizeof(valopt); 

    if (!(this->edgercs_active) || doug_command.empty()) return 0;

    len = doug_command.length();
    if (len >= MAX_SEND_SIZE)
    {
        debug_msg("Command too long.  Ignoring...");
        return 1;
    }

    // Set timeout interval for later use
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    // Loop through available addresses until we get a socket connection
    for (addr = this->server_addr_info; addr && command_socket < 0; addr = addr->ai_next)
    {
        // Try creating the socket
        command_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (command_socket >= 0)
        {
            // Set socket to non-blocking mode
            arg = fcntl(command_socket, F_GETFL, 0x0); 
            arg |= O_NONBLOCK; 
            fcntl(command_socket, F_SETFL, arg); 

            if (connect(command_socket, (struct sockaddr *)addr->ai_addr, addr->ai_addrlen) < 0)
            {
                if (errno == EINPROGRESS)
                {
                    FD_ZERO(&myset);
                    FD_SET(command_socket, &myset);
                    if (select(command_socket+1, 0x0, &myset, 0x0, &timeout) > 0)
                    {
                        getsockopt(command_socket, SOL_SOCKET, SO_ERROR, (void *)(&valopt), &valopt_len);
                        if (valopt)
                        {
                            debug_msg("Error " << valopt << " in connection: " << strerror(valopt));
                            close(command_socket);
                            command_socket = -1;
                        }
                    }
                    else
                    {
                        debug_msg("Timeout or error "<< valopt << ": " << strerror(valopt));
                        close(command_socket);
                        command_socket = -1;
                    }
                }
                else
                {
                    debug_msg("Error " << errno << " connecting:" << strerror(errno));
                    close(command_socket);
                    command_socket = -1;
                }
            }
        }
    }

    if (command_socket < 0)
    {
        debug_msg("Couldn't create socket");
        return 1;
    }

    // Set socket to blocking mode again
    arg = fcntl(command_socket, F_GETFL, 0x0); 
    arg &= (~O_NONBLOCK); 
    fcntl(command_socket, F_SETFL, arg); 

    // Try using a receive timeout, in case the EDGE RCS hangs, we don't wait forever.
    setsockopt(command_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));

    // Read the server version off the socket
    if (this->read_rcs_message(command_socket, rcs_version) == -1)
    {
        close(command_socket);
        return 1;
    }

    if (write(command_socket, doug_command.c_str(), len) != (int)len)
    {
        debug_msg("Partial/failed write");
        close(command_socket);
        return 1;
    }

    // Signal the server that we are done sending
    shutdown(command_socket, SHUT_WR);

    // Now read exec result of command coming back (works with rcs server after feb 17, 2010)
    if (this->read_rcs_message(command_socket, command_result) == -1)
    {
        close(command_socket);
        return 1;
    }

    close(command_socket);
    return 0;
}

/* Reads a single message from the remote commanding server.. */
int EdgeRcsComm::read_rcs_message(int socket, char **rcs_response_string)
{
    char incoming_char;
    int nread = 0, buf_size = BUF_SIZE_INCREMENT;
    bool more_to_read = true;

    if (rcs_response_string)
    {
        *rcs_response_string = (char *)realloc(*rcs_response_string, buf_size);
        if (!(*rcs_response_string))
        {
            error_msg("Memory allocation error");
            return -1;
        }
    }

    // Read off one char at a time until we hit END_OF_MSG_CHAR
    while (more_to_read)
    {
        if (read(socket, &incoming_char, 1) != 1)
        {
            debug_msg("Failed to read character from socket");
            return -1;
        }

        if (incoming_char == END_OF_MSG_CHAR) more_to_read = false;
        else
        {
            if (rcs_response_string)
            {
                // If the result container is too small, grow it
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
