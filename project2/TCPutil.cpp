#include "TCPutil.h"
#include "logerr.h"

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

void htonPacket(simpleTCP& packet)
{
    packet.setSeqNum(htons(packet.getSeqNum()));
    packet.setAckNum(htons(packet.getAckNum()));
    packet.setWindow(htons(packet.getWindow()));
    packet.setFlags(htons(packet.getFlags()));
}

void ntohPacket(simpleTCP& packet)
{
    packet.setSeqNum(ntohs(packet.getSeqNum()));
    packet.setAckNum(ntohs(packet.getAckNum()));
    packet.setWindow(ntohs(packet.getWindow()));
    packet.setFlags(ntohs(packet.getFlags()));
}

simpleTCP makePacket_ton(uint16_t seq_num, uint16_t ack_num, uint16_t window, uint16_t flags,
                         const char *message, int size)
{
    struct simpleHeader header;
    header.seq_num = seq_num;
    header.ack_num = ack_num;
    header.window = window;
    header.flags = flags;

    simpleTCP packet(header, message, size);
    htonPacket(packet);
    return packet;
}

/* NETWORK-RELATED */

bool sendAll(int sockfd, const void *buf, size_t length, int flags,
             const struct sockaddr *dest_addr, socklen_t dest_len)
{
    size_t total = 0;
    size_t bytesLeft = length;

    while (total < length)
    {
        int sent = sendto(sockfd, (const char*)buf + total, bytesLeft, flags, dest_addr, dest_len);
        if (sent == -1)
            return false;
        total += sent;
        bytesLeft -= sent;
    }
    //_DEBUG("Total sent: " + to_string(total) + " bytes");
    return true;
}

bool sendAck(int sockfd, const struct sockaddr *server_addr, socklen_t server_addr_length,
             simpleTCP& ack_packet, bool retransmission)
{
    assert(ack_packet.getSegmentSize() == 8);
    cout << "Sending ACK packet " << ntohs(ack_packet.getAckNum());
    if (retransmission)
        cout << " Retransmission";
    cout << endl;

    if (!sendAll(sockfd, (void *)&ack_packet, ack_packet.getSegmentSize(), 0,
                 server_addr, server_addr_length))
    {
        perror("sendto() error in client while sending ACK");
        return false;
    }
    return true;
}

int recvPacket_toh(int sockfd, simpleTCP& packet, struct sockaddr *server_addr, socklen_t *server_addr_length)
{
    int nbytes = recvfrom(sockfd, (void *)&packet, MAX_SEGMENT_SIZE, 0,
                          server_addr, server_addr_length);
    int payload_size = nbytes - packet.getHeaderSize();
    if (payload_size >= 0)
        packet.setPayloadSize(payload_size);
    ntohPacket(packet);

    return nbytes;
}

int timeSocket(int sockfd, struct timeval *timeout)
{
    fd_set listening_socket;
    FD_ZERO(&listening_socket);
    FD_SET(sockfd, &listening_socket);
    return select(sockfd + 1, &listening_socket, NULL, NULL, timeout);
}
