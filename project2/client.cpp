#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <thread>
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
    
    int seq_num, ack_num, cong_window;
    simpleTCP recv_packet;
    struct simpleHeader header;
    int nbytes;
    bool init_ack;
    bool retransmission = false;
    
    srand(time(NULL));
    seq_num = rand() % MAX_SEQ_NUM; // random initial sequence number
    ack_num = 0; // initally unusued
    cong_window = INIT_CONG_SIZE; // clients don't need congestion window, so this can be ignored
    init_ack = false;

    // sends syn packet
    header.seq_num = seq_num;
    header.ack_num = ack_num;
    header.window = cong_window;
    header.flags = F_SYN;
    simpleTCP syn_packet = simpleTCP(header, "", 0);
    seq_num++; // increment seq_num by 1 since 0 payload (not sure if this is right)
    if (sendto(sockfd, (void *)&syn_packet, sizeof(syn_packet), 0,
               server_addr, server_addr_length) == -1)
    {
        perror("sendto() error in client while sending SYN");
    }
    
    // receives syn-ack packet, resending syn if not received in time
    
    while (!init_ack)
    {
        fd_set listening_socket;
        FD_ZERO(&listening_socket);
        FD_SET(sockfd, &listening_socket);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = INIT_RTO * 1000;

        if (select(sockfd + 1, &listening_socket, NULL, NULL, &timeout) > 0)
        {
            if ((nbytes = recvfrom(sockfd, (void *)&recv_packet, sizeof(recv_packet), 0,
                                   server_addr, &server_addr_length)) == -1)
            {
                perror("recvfrom() error in client while processing SYN-ACK/SYN");
            }
            else
            {
                init_ack = recv_packet.getACK() && recv_packet.getSYN();
                ack_num = (recv_packet.getSeqNum() + 1) % MAX_SEQ_NUM;
            }
        }
        else
        {
            if (sendto(sockfd, (void *)&syn_packet, sizeof(syn_packet), 0,
                       server_addr, server_addr_length) == -1)
            {
                perror("sendto() error in client while sending SYN");
            }
        }
    }
    
    // sends ack packet
    header.seq_num = seq_num;
    header.ack_num = ack_num;
    header.window = cong_window;
    header.flags = F_ACK;
    syn_packet = simpleTCP(header, "", 0);
    seq_num++; // increment seq_num by 1 since 0 payload (not sure if this is right)
    if (sendto(sockfd, (void *)&syn_packet, sizeof(syn_packet), 0,
               server_addr, server_addr_length) == -1)
    {
        perror("sendto() error in client while sending ACK");
    }
    else
    {
        if (!retransmission)
        {
            cout << "Sending ACK packet " << recv_packet.getAckNum() << endl;
            retransmission = true;
        }
        else
        {
            cout << "Sending ACK packet " << recv_packet.getAckNum() << " Retransmission" << endl;
        }
    }
    
    // file stuff and fin/fin-ack/ack stuff here
    
}





