#include <cstring>
#include <arpa/inet.h>

#include "TCPutil.h"

using namespace std;

int getIpv4(const string& host, int port, struct addrinfo **result)
{
    struct addrinfo hints;
    
    // Hints: IPv4 and UDP
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    return getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, result);
}

void htonPacket(struct simpleTCP& packet)
{
    packet.setSeqNum(htons(packet.getSeqNum()));
    packet.setAckNum(htons(packet.getAckNum()));
    packet.setWindow(htons(packet.getWindow()));
    packet.setFlags(htons(packet.getFlags()));
}

void ntohPacket(struct simpleTCP& packet)
{
    packet.setSeqNum(ntohs(packet.getSeqNum()));
    packet.setAckNum(ntohs(packet.getAckNum()));
    packet.setWindow(ntohs(packet.getWindow()));
    packet.setFlags(ntohs(packet.getFlags()));
}
