#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>

#include "simpleTCP.h"
#include "constants.h"
#include "TCPutil.h"
#include "logerr.h"

using namespace std;

static void errorAndExit(const string& msg)
{
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

/* GLOBAL VARIABLES */
static int seq_num = 0;
static int ack_num = 0;
static int rto = INIT_RTO * 1000;

// HANDSHAKE:
// - send SYN; if not successful, wait and continue
// - receive SYN/ACK; if not received, wait and continue
// - send ACK; set this as last_ack; break
// Returns the last ACK packet sent.
static simpleTCP handshake(int sockfd, struct sockaddr *server_addr, socklen_t server_addr_length)
{
    int nbytes;
    simpleTCP packet;

    while (true)
    {
        // Send SYN
        _DEBUG("Sending SYN packet");

        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_SYN, "", 0);
        assert(packet.getSegmentSize() == 8);
        if (sendto(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                   server_addr, server_addr_length) == -1)
        {
            perror("sendto() error in client while sending SYN");
            usleep(rto);
            continue;
        }

        // Receive SYN/ACK packet
        _DEBUG("Receiving SYN/ACK packet");

        fd_set listening_socket;
        FD_ZERO(&listening_socket);
        FD_SET(sockfd, &listening_socket);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = rto;
        
        if (select(sockfd + 1, &listening_socket, NULL, NULL, &timeout) > 0)
        {
            nbytes = recvPacket(sockfd, packet, server_addr, &server_addr_length);
            ntohPacket(packet);

            // Packet must have at least a header and be SYN/ACK
            if (nbytes < packet.getHeaderSize() || !packet.getACK() || !packet.getSYN())
            {
                perror("recvfrom() error in client while receiving SYN/ACK");
                usleep(rto);
                continue;
            }
        }
        else
        {
            _DEBUG("Timeout receiving SYN/ACK packet");
            usleep(rto);
            continue;
        }
        
        // Send ACK packet
        _DEBUG("Sending ACK packet");

        ack_num = (packet.getSeqNum() + 1) % MAX_SEQ_NUM;
        seq_num = (seq_num + 1) % MAX_SEQ_NUM;

        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, "", 0);
        sendAck(sockfd, server_addr, server_addr_length, packet, false);
        return packet;
    }    
}

static bool isValidSeq(int a, int b)
{
    if (b - a <= MAX_SEQ_NUM/2 || b - a + MAX_SEQ_NUM <= MAX_SEQ_NUM/2)
        return true;
    return false;
}

// RECEIVE FILE:
// - receive a data packet
// - if out-of-order packet or timeout, resend last_ack_packet
// - else, create a new ACK and send it; set this as the last_ack_packet
// - if data packet has FIN set, break
static void receiveFile(int sockfd, struct sockaddr *server_addr, socklen_t server_addr_length,
                        simpleTCP last_ack_packet)
{
    int nbytes;
    simpleTCP packet;
    ofstream ofs("out.data");

    if (!ofs)
        errorAndExit("Can't create out.data");

    while (true)
    {
        fd_set listening_socket;
        FD_ZERO(&listening_socket);
        FD_SET(sockfd, &listening_socket);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = rto;
        
        if (select(sockfd + 1, &listening_socket, NULL, NULL, &timeout) > 0)
        {
            nbytes = recvPacket(sockfd, packet, server_addr, &server_addr_length);
            ntohPacket(packet);

            // Packet must have at least a header and an ACK
            if (nbytes >= packet.getHeaderSize() && packet.getACK())
            {
                if (packet.getSeqNum() == ack_num)  // In-order packet
                {
                    ack_num = (ack_num + packet.getPayloadSize()) % MAX_SEQ_NUM;
                    packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, "", 0);
                    last_ack_packet = packet;
                    sendAck(sockfd, server_addr, server_addr_length, packet, false);

                    ofs.write(packet.getMessage(), packet.getPayloadSize());
                    if (packet.getFIN())
                        break;
                }
                else if (isValidSeq(ack_num, packet.getSeqNum()))   // Old packet
                {
                    sendAck(sockfd, server_addr, server_addr_length, last_ack_packet, true);
                }
                else    // Out-of-order packet
                {
                    // Drop for now
                }
            }
            else        // Malformed packet (or none)
            {
                _ERROR("Receiving data packet");
                sendAck(sockfd, server_addr, server_addr_length, last_ack_packet, true);
            }
        }
        else            // Timeout
        {
            _DEBUG("Timeout receiving data packet");
            sendAck(sockfd, server_addr, server_addr_length, last_ack_packet, true);
        }
    }
    ofs.close();
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        perror("Not enough arguments");
    }

    string host = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr *server_addr;
    socklen_t server_addr_length;

    // Connect to server and store connection (addrinfo) information
    {
        struct addrinfo *servAddr = NULL, *addr;
        int status;

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
            break;
        }
        
        if (addr == NULL)
            errorAndExit("No connection possible for host: " + host);

        // Free memory later
        server_addr = (struct sockaddr *)addr->ai_addr;
        server_addr_length = addr->ai_addrlen;
    }
    
    simpleTCP last_ack_packet = handshake(sockfd, server_addr, server_addr_length);

    receiveFile(sockfd, server_addr, server_addr_length, last_ack_packet);

    // TEARDOWN:
}
