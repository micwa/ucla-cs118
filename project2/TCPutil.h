#ifndef TCPutil_h
#define TCPutil_h

#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "simpleTCP.h"

int getIpv4(const std::string& host, int port, struct addrinfo **result);

void htonPacket(simpleTCP& packet);

void ntohPacket(simpleTCP& packet);

// Creates a simpleTCP packet from the given arguments, and calls htonPacket(packet)
// before returning it.
simpleTCP makePacket_ton(uint16_t seq_num, uint16_t ack_num, uint16_t window, uint16_t flags,
                         const char *message, int size);

#endif
