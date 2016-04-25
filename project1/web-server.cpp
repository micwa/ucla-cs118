#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <thread>

const int BACKLOG = 20; // convention
const int WAIT_SEC = 3;
const int WAIT_USEC = 0;

void response()
{
    //accept here, and do file stuff
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        cout << "Invalid number of arguments!\n";
        return 1;
    }

    struct addrinfo addrhint;
    struct addrinfo *addrserv;

    memset(&addrhint, 0, sizeof(addrinfo));
    addrhint.ai_family = AF_INET;
    addrhint.ai_socktype = SOCK_STREAM;

    int port_num = stoi(argv[2]);

    if (getaddrinfo(argv[1], port, &addrhint, &addrserv) != 0)
    {
        _DEBUG("Error in getaddrinfo");
    }

    int sfd = socket(addrserv->ai_family, addrserv->ai_socktype, addrserv->ai_protocol);

    // if "Address already in use." is a problem look in 5.3 setsockopt
    int yes = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        _DEBUG("Error in setsockopt");
        return 1;
    }

    bind(sfd, addrserv->ai_addr, addrserv->ai_addrlen);

    if (listen(sfd, BACKLOG) == -1)
    {
        _DEBUG("Error in listen");
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = WAIT_SEC;
    tv.tv_usec = WAIT_USEC;

    fd_set listening_socket;
    FD_ZERO(&listening_socket);
    FD_SET(sfd, &listening_socket);

    for (;;)
    {
        if (select(sfd+1, &listening_socket, NULL, NULL, &tv) > 0)
        {
            thread connection(response);
            connection.detach();
        }
        else
        {
            _DEBUG("Nothing received in time specificed in timeval tv");
        }
    }


    return 1; // should never get here
}

Server
======

Steps:
    - store PATH (directory) as variable
    - getaddrinfo() for IP
    - socket() to create socket listen_socket
    - bind() to bind to IP:PORT
    - listen() for connection on listen_socket
    - while true:
        [multi-threaded]
        - tmp_sock = accept() the connection
        - spawn thread new_thread to handleConnection(tmp_sock)
        - new_thread.detach()               // So no need to join()
        [single-threaded, using select()]
        - maintain map<int, time_t> openConns of open connections,
          mapping socket -> (time last active)
        - select(), with timeout, until socket sock_ready is available
        - if sock_ready == listening socket:
            - tmp_sock = accept() the connection
            - openConns[tmp_sock] = getCurrentTime()
        - else if sock_ready == socket in openConns:
            [persistent, pipelined]
            - recv() the request
            - create HttpResponse
            - send() the response
            - if request had "Connection: close", close() the connection
              (e.g., set openConns[sock_ready] = -1);
            - else, update openConns[sock_ready] to getCurrentTime()
            [non-persistent - it's more complicated than persistent, so not going to bother]
        - close() all connections in openConns that are "timed out"
    where
    handleConnection(int sockfd)
        [non-persistent]
        - recv(), with timeout, the request
        - create HttpResponse
        - send() the response
        - close() the connection
        [persistent, pipelined]
        - while true:
            - recv(), with timeout, the request;
              if timeout or EOF, close() the connection and break
            - create HttpResponse
            - send() the response
            - if request had "Connection: close", close() the connection and break

