#ifndef TCPutil_h
#define TCPutil_h

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "simpleTCP.h"

int getIpv4(const std::string& host, int port, struct addrinfo **result);

void htonPacket(struct simpleTCP& packet);

void ntohPacket(struct simpleTCP& packet);

#endif
