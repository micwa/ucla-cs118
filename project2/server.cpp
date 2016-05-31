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

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        perror("Not enough arguments");
    }

    int port = atoi(argv[1]);
    string filename = argv[2];

    // just proj1 stuff that I modified, check it to see if I modified it correctly
    int sockfd;

    struct addrinfo *servAddr = NULL, *addr;
    int status, yes = 1;
    string host = "localhost"; // assuming that it's always localhost as proj2 doesn't specify
    if ((status = getIpv4(host, port, &servAddr)) != 0)
        perror("getaddrinfo: " + gai_strerror(status));
        
    // Try all IPs
    for (addr = servAddr; addr != NULL; addr = addr->ai_next)
    {
        if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
        {
            perror("socket() error");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt() error");
            continue;
        }
        if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1)
        {
            perror("bind() error");
            continue;
        }
        break;
    }
        
    freeaddrinfo(servAddr);
    if (addr == NULL)
        errorAndExit("No connection possible for host: " + host);
 
    
    /* This is what I wrote before I realized I could and should just modify proj1 stuff
    int sockfd;
    if (sockfd = socket(AF_INET, SOCK_DGRAM, 0) == -1)

    {
         perror("socket() error");
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
         perror("setsockopt() error");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
         perror("bind() error");
    }
    */

    int seq_num, ack_num, cong_window;
    simpleTCP recv_packet;
    struct sockaddr client_addr;
    socklen_t client_addr_length;
    struct simpleHeader header;
    int nbytes;
    bool init_syn, init_ack;
    struct timeval timeout;
    
    for (;;)
    {
        srand(time(NULL));
        seq_num = rand() % MAX_SEQ_NUM; // random initial sequence number
        cong_window = INIT_CONG_SIZE;
        timeout.tv_sec = 0;
        timeout.tv_usec = INIT_RTO * 1000;
        init_syn = false;
        init_ack = false;
        
        // receives syn packet
        while (!init_syn)
        {
            if (nbytes = recvfrom(sockfd, (void *)&recv_packet, sizeof(recv_packet), 0, client_addr, &client_addr_length) == -1)
            {
                perror("recvfrom() error in server while processing SYN");
            }
            else
            {
                init_syn = recv_packet.getSYN();
                ack_num = (seq_num + 1) % MAX_SEQ_NUM; // payload is 0 bytes, but can't do + 0
            }
        }

        // sends syn-ack packet
        header.seq_num = seq_num;
        header.ack_num = ack_num;
        header.window = cong_window;
        header.flags = F_SYN | F_ACK;
        simpleTCP synack_packet = simpleTCP(header, "", 0);
        seq_num++; // increment seq_num by 1 since 0 payload (not sure if this is right)
        if (sendto(sockfd, (void *)&synack_packet, sizeof(synack_packet), 0, (struct sockaddr *)&client_addr, &client_addr_length == -1))
        {
            perror("sendto() error in server while sending SYN-ACK/SYN");
        }
        
        // receives ack packet, resending syn-ack if not received in time
        
        while (!init_ack)
        {
            fd_set listening_socket;
            FD_ZERO(&listening_socket);
            FD_SET(sockfd, &listening_socket);
            // implements timeout - don't use select(), it can change timeout
            if (pselect(sockfd + 1, &listening_socket, NULL, NULL, &timeout, NULL) > 0)
            {
                if (nbytes = recv(sockfd, (void *)&recv_packet, sizeof(recv_packet), 0) == -1)
                {
                    perror("recv() error in server while processing ACK");
                }
                else
                {
                    init_ack = recv_packet.getACK();
                    ack_num = (recv_packet.seq_num + 1) % MAX_SEQ_NUM; // payload is 0 bytes, but can't do + 0
                    cout << "Receiving ACK packet " << recv_packet.getAckNum() << endl;
                }
            }
            else
            {
                if (sendto(sockfd, (void *)&synack_packet, sizeof(synack_packet), 0, (struct sockaddr *)&client_addr, &client_addr_length == -1))
                {
                    perror("sendto() error in server while sending SYN-ACK/SYN");
                }
            }
        }
        
        // file stuff and fin/fin-ack/ack stuff here
        
    }
}



