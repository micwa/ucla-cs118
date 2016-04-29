#include "constants.h"
#include "FileResponse.h"
#include "HttpRequest.h"
#include "WebUtil.h"
#include "logerr.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

static bool serverOn = true;

static void errorAndExit(const string& msg)
{
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

void handleConnection(int sockfd, const string& baseDirectory)
{
    // Persistent connection; close connection only if EOF/timeout
    while (true)
    {
        FileResponse response;
        int status;

        // Receive HTTP request, with timeout
        status = response.recvRequest(sockfd);
        if (status == 0)
        {
            _DEBUG("Client closed connection/timeout");
            break;
        }
        else if (status == -1)  // If malformed, keep connection open
        {
            _ERROR("Request was malformed");
        }

        // Send response, and close connection if request was nonpersistent
        string value;
        if (response.sendResponse(sockfd, baseDirectory))
            _DEBUG("Response sent successfully");
        else
            _DEBUG("Error sending response");

        if (status > 0)
        {
            HttpRequest *request = response.getRequest();
            if (request->getHttpVersion() != HTTP_VERSION_11 ||
                (request->getHeader("Connection", value) && value == "close"))
                break;
        }
    }
    _DEBUG("CONNECTION CLOSED: " + to_string(sockfd));
    close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        errorAndExit("Invalid number of arguments");

    // Parse host, port, baseDirectory; also check for errors
    string host = argv[1];
    int port;
    string baseDirectory;

    port = atoi(argv[2]);
    if (port < 1 || port > 65535)
        errorAndExit("Invalid port number");
    {
        struct stat s;
        int status = stat(argv[3], &s);
        if (status == 0 && S_ISDIR(s.st_mode))
            baseDirectory = argv[3];
        else
            errorAndExit("Invalid directory");
    }

    // Bind and listen on host:port
    int sockfd;
    {
        struct addrinfo *servAddr = NULL, *addr;
        int status, yes;

        if ((status = getIpv4(host, port, &servAddr)) != 0)
            errorAndExit(string("getaddrinfo: ") + gai_strerror(status));

        // Try all IPs
        for (addr = servAddr; addr != NULL; addr = addr->ai_next)
        {
            if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
            {
                perror("socket() error");
                continue;
            }
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
            {
                perror("setsockopt() error");
                continue;
            }
            if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1)
            {
                perror("bind() error");
                continue;
            }
            break;
        }

        freeaddrinfo(servAddr);
        if (addr == NULL)
            errorAndExit("No connection possible for host: " + host);
    }
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen() error");
        return 1;
    }

    // Keep accept()ing connections as long as serverOn == true
    _DEBUG("Waiting for connections...");
    while (serverOn)
    {
        struct timeval tv;
        tv.tv_sec = LISTEN_WAIT_SEC;
        tv.tv_usec = LISTEN_WAIT_USEC;

        fd_set listening_socket;
        FD_ZERO(&listening_socket);
        FD_SET(sockfd, &listening_socket);

        if (select(sockfd + 1, &listening_socket, NULL, NULL, &tv) > 0)
        {
            int new_fd = accept(sockfd, NULL, 0);
            thread connection(handleConnection, new_fd, baseDirectory);
            connection.detach();
        }
        else
        {
            _DEBUG("Nothing received in: " +
                   to_string(LISTEN_WAIT_SEC + (double)LISTEN_WAIT_USEC / 1000000) +
                   " seconds");
        }
    }
    _DEBUG("Server shutting down");
    return 0;
}
