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

const int BACKLOG = 20; // convention

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    cout << "Invalid number of arguments!\n";
    return 1;
  }

  struct addrinfo addrhint;
  struct addrinfo *addrserv;

  memset(&addrhint, 0, sizeof(addrinfo));
  addrhint.ai_family = AF_INET;
  addrhint.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(argv[0], argv[1], &addrhint, &addrserv) != 0)
  {
    _DEBUG("Error in getaddrinfo");
  }

  int sfd = socket(addrserv->ai_family, addrserv->ai_socktype, addrserv->ai_protocol);

  // if "Address already in use." is a problem look in 5.3 setsockopt
  bind(sfd, addrserv->ai_addr, addrserv->ai_addrlen);

  listen(sfd, BACKLOG);

  //accept here
}
