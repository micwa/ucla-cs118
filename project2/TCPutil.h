#ifndef TCPutil_h
#define TCPutil_h

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

int getIpv4(const std::string& host, int port, struct addrinfo **result);

#endif
