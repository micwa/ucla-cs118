#include "constants.h"
#include "FileResponse.h"
#include "HttpRequest.h"
#include "WebUtil.h"
#include "logerr.h"

#include <csignal>
#include <cstring>
#include <ctime>
#include <iostream>
#include <thread>
#include <unordered_map>
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

static void sigintHandler(int)
{
    // Turn off the server
    serverOn = false;
}

static time_t getCurrentTime()
{
    return time(NULL);
}

// Returns true if the request is persistent, and false if not.
static bool handleRequest(int sockfd, const string& baseDirectory)
{
    FileResponse response;
    int status;

    status = response.recvRequest(sockfd);
    if (status == 0)
    {
        _DEBUG("Client closed connection/timeout");
        return false;
    }
    else if (status == -1)
    {
        _ERROR("Request was malformed");
    }

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
            return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        errorAndExit("Invalid number of arguments");

    // Install signal handler
    signal(SIGINT, sigintHandler);

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
    int listenfd;
    {
        struct addrinfo *servAddr = NULL, *addr;
        int status, yes = 1;

        if ((status = getIpv4(host, port, &servAddr)) != 0)
            errorAndExit(string("getaddrinfo: ") + gai_strerror(status));

        // Try all IPs
        for (addr = servAddr; addr != NULL; addr = addr->ai_next)
        {
            if ((listenfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
            {
                perror("socket() error");
                continue;
            }
            if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
            {
                perror("setsockopt() error");
                continue;
            }
            if (bind(listenfd, addr->ai_addr, addr->ai_addrlen) == -1)
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
    if (listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error");
        return 1;
    }

    // fd_set for select()
    fd_set readsetAll;  // To keep track of which sockets are readable
    fd_set readset;     // Used as paramter
    FD_ZERO(&readsetAll);
    FD_ZERO(&readset);
    FD_SET(listenfd, &readsetAll);

    // Connection management
    int maxSock = listenfd;
    unordered_map<int, time_t> openConns;

    // Keep accept()ing connections as long as serverOn == true
    _DEBUG("Waiting for connections...");
    while (serverOn)
    {
        struct timeval tv;
        tv.tv_sec = LISTEN_WAIT_SEC;
        tv.tv_usec = LISTEN_WAIT_USEC;

        int num_ready;
        readset = readsetAll;
        if ((num_ready = select(maxSock + 1, &readset, NULL, NULL, &tv)) == 0)
        {
            _DEBUG("Nothing received in: " +
                   to_string(LISTEN_WAIT_SEC + (double)LISTEN_WAIT_USEC / 1000000) +
                   " seconds");
        }
        else if (num_ready == -1)
        {
            perror("select() error");
        }
        else
        {
            // Check listening socket
            if (FD_ISSET(listenfd, &readset))
            {
                int new_fd = accept(listenfd, NULL, 0);
                openConns[new_fd] = getCurrentTime();
                FD_SET(new_fd, &readsetAll);
                maxSock = max(maxSock, new_fd);

                _DEBUG("Accepted connection: " + to_string(new_fd));
                if (num_ready == 1)
                    continue;
            }
            // Go through all connection sockets
            for (auto it = openConns.begin(); it != openConns.end(); ++it)
            {
                int sockfd = it->first;
                if (FD_ISSET(sockfd, &readset))
                {
                    bool isPersistent = handleRequest(sockfd, baseDirectory);
                    if (isPersistent)
                        it->second = getCurrentTime();
                    else
                        it->second = 0;     // "time out" the socket
                }
            }
        }

        // Close all timed-out connections
        for (auto it = openConns.begin(); it != openConns.end();)
        {
            int sockfd      = it->first;
            time_t oldTime  = it->second;
            time_t currTime = getCurrentTime();
            double diff     = difftime(currTime, oldTime);

            if (diff >= RECV_TIMEOUT_SECS)
            {
                _DEBUG("CONNECTION CLOSED: " + to_string(sockfd));
                close(sockfd);
                FD_CLR(sockfd, &readsetAll);
                it = openConns.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    _DEBUG("Server shutting down");
    return 0;
}
