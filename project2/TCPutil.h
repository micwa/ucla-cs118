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

// Sends length bytes from buf. Returns true if the data is sent successfully, and false if not.
bool sendAll(int sockfd, const void *buf, size_t length, int flags,
             const struct sockaddr *dest_addr, socklen_t dest_len);

// Send an ACK packet (printing out the ACK number) and returns the result of sendto().
// FOR CLIENT
bool sendAck(int sockfd, const struct sockaddr *server_addr, socklen_t server_addr_length,
             simpleTCP& ack_packet, bool retransmission);

// Receive MAX_SEGMENT_SIZE bytes into the given packet, and set its payload size.
// Returns the return value of recvfrom().
int recvPacket_toh(int sockfd, simpleTCP& packet, struct sockaddr *server_addr, socklen_t *server_addr_length);

#endif
