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

    int port = atoi(argv[1]);
    string filename = argv[2];

    // Bind and listen on host port
    int sockfd;
    {
        struct addrinfo *servAddr = NULL, *addr;
        int status, yes = 1;
        string host = "localhost";
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
        /* don't need listen() for UDP
        if (listen(sockfd, BACKLOG) == -1)
        {
            perror("listen() error");
            return 1;
        }
        */
    }

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
                    ack_num = (recv_packet.seq_num + 1) % MAX_SEQ_NUM; 
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



