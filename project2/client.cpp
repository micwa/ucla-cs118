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
#include "TCPrto.h"
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
static TCPrto rtoObj;

// HANDSHAKE:
// - send SYN (assume it's sent)
// - receive SYN/ACK; if invalid or not received, continue
// - send ACK; set this as last_ack; break
// Returns the last ACK packet sent.
static simpleTCP handshake(int sockfd, struct sockaddr *server_addr, socklen_t server_addr_length)
{
    simpleTCP packet;
    bool retransmission = false;

    while (true)
    {
        // Send SYN
        cout << "Sending SYN packet " << seq_num;
        if (retransmission)
            cout << " Retransmission";
        cout << endl;
        retransmission = true;

        struct timeval sent_time, recv_time, diff_time;
        gettimeofday(&sent_time, NULL);
        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_SYN, "", 0);

        if (!sendAll(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                   server_addr, server_addr_length))
        {
            perror("sendAll() sending SYN");
        }

        // Receive SYN/ACK packet
        struct timeval timeout = rtoObj.getRto();
        if (timeSocket(sockfd, &timeout) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, packet, server_addr, &server_addr_length);

            // Packet must have at least a header, be SYN/ACK, and have the proper sequence number
            if (nbytes < packet.getHeaderSize() || !packet.getACK() || !packet.getSYN())
            {
                _ERROR("Receiving SYN/ACK packet for handshake");
                continue;
            }
        }
        else
        {
            _DEBUG("Timeout receiving SYN/ACK packet");
            rtoObj.rtoTimeout();
            continue;
        }
        cout << "Receiving SYN/ACK packet" << endl;
        gettimeofday(&recv_time, NULL);
        timersub(&recv_time, &sent_time, &diff_time);
        rtoObj.srtt(diff_time);
        
        // Send ACK packet
        ack_num = (packet.getSeqNum() + 1) % MAX_SEQ_NUM;
        seq_num = (seq_num + 1) % MAX_SEQ_NUM;

        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, "", 0);
        sendAck(sockfd, server_addr, server_addr_length, packet, false);
        return packet;
    }    
}

static bool isValidSeq(int a, int b)
{
    if (b - a + 1 <= MAX_SEQ_NUM/2 || b - a + 1 + MAX_SEQ_NUM <= MAX_SEQ_NUM/2)
        return true;
    return false;
}

// RECEIVE FILE:
// - receive a data packet
// - if old data packet, resend last_ack_packet
// - else if proper packet, create a new ACK and send it; set this as the last_ack_packet
// - if data packet has FIN set, break
static void receiveFile(int sockfd, struct sockaddr *server_addr, socklen_t server_addr_length,
                        simpleTCP last_ack_packet)
{
    simpleTCP packet;
    ofstream ofs("received.data");

    if (!ofs)
        errorAndExit("Can't create out.data");

    while (true)
    {
        int nbytes = recvPacket_toh(sockfd, packet, server_addr, &server_addr_length);
        int packet_seq = packet.getSeqNum();

        // Packet must have at least a header and an ACK
        if (nbytes >= packet.getHeaderSize() && packet.getACK())
        {
            cout << "Receiving data packet " << packet_seq << endl;
            if (packet_seq == ack_num)  // In-order packet
            {
                ofs.write(packet.getMessage(), packet.getPayloadSize());

                ack_num = (ack_num + packet.getPayloadSize()) % MAX_SEQ_NUM;
                if (packet.getFIN())
                    ack_num = (ack_num + 1) % MAX_SEQ_NUM;

                last_ack_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, "", 0);
                sendAck(sockfd, server_addr, server_addr_length, last_ack_packet, false);

                if (packet.getFIN())
                    break;
            }
            else if (isValidSeq(ack_num, packet_seq))   // Old packet
            {
                _DEBUG("Old packet")
                sendAck(sockfd, server_addr, server_addr_length, last_ack_packet, true);
            }
            else    // Out-of-order packet
            {
                _DEBUG("Out-of-order packet");
                sendAck(sockfd, server_addr, server_addr_length, last_ack_packet, true);
            }
        }
        else        // Malformed packet (or recvfrom() error)
        {
            _ERROR("Receiving data packet");
        }
    }
    ofs.close();
}

// TEARDOWN:
// - send FIN/ACK
// - receive ACK
// - if timeout or invalid, continue; else break
static void teardown(int sockfd, struct sockaddr *server_addr, socklen_t server_addr_length)
{
    simpleTCP packet;
    bool retransmission = false;

    while (true)
    {
        // Send FIN/ACK
        cout << "Sending SYN/ACK packet " << ack_num;
        if (retransmission)
            cout << " Retransmission";
        cout << endl;
        retransmission = true;

        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK|F_SYN, "", 0);
        assert(packet.getSegmentSize() == 8);

        if (!sendAll(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                     server_addr, server_addr_length))
        {
            perror("sendto() error in client while sending FIN/ACK");
        }

        // Receive ACK
        struct timeval timeout = rtoObj.getRto();
        
        if (timeSocket(sockfd, &timeout) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, packet, server_addr, &server_addr_length);

            // Packet must have proper sequence number and ACK set
            if (nbytes < packet.getHeaderSize() || !packet.getACK() ||
                packet.getSeqNum() != ack_num)
            {
                _ERROR("Receiving ACK packet for teardown");
                continue;
            }
        }
        else            // Timeout
        {
            _DEBUG("Timeout receiving ACK packet");
            rtoObj.rtoTimeout();
            continue;
        }

        // Everything OK
        cout << "Receiving ACK packet " << packet.getAckNum() << endl;
        break;
    }
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
    struct addrinfo *server_addr_info = NULL, *addr;
    struct sockaddr *server_addr;
    socklen_t server_addr_length;

    // Connect to server and store connection (addrinfo) information
    {
        int status;

        if ((status = getIpv4(host, port, &server_addr_info)) != 0)
            errorAndExit(string("getaddrinfo: ") + gai_strerror(status));
        
        // Try all IPs
        for (addr = server_addr_info; addr != NULL; addr = addr->ai_next)
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

    teardown(sockfd, server_addr, server_addr_length);

    freeaddrinfo(server_addr_info);
    _DEBUG("Client exiting");
    return 0;
}
