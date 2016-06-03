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
        string host = "0.0.0.0";
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
    }
    
    int seq_num, ack_num, cong_window;
    simpleTCP recv_packet;
    struct sockaddr client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    int nbytes;
    bool init_syn, data_left;
    
    srand(time(NULL));
    seq_num = 0;//rand() % MAX_SEQ_NUM; // random initial sequence number
    cong_window = INIT_CONG_SIZE;
    init_syn = false;
    
    cout << "about to receive syn packet" << endl;
    
    // receives syn packet
    while (!init_syn)
    {
        if ((nbytes = recvfrom(sockfd, (void *)&recv_packet, sizeof(simpleHeader), 0,
                               &client_addr, &client_addr_length)) == -1)
        {
            perror("recvfrom() error in server while processing SYN");
        }
        else
        {
            ntohPacket(recv_packet);
            init_syn = recv_packet.getSYN();
            ack_num = (seq_num + 1) % MAX_SEQ_NUM;
        }
    }
    
    while (true)
    {
        cout << "sending syn-ack packet" << endl;
        // sends syn-ack packet
        simpleTCP synack_packet = makePacket_ton(seq_num, ack_num, cong_window, F_SYN | F_ACK, "", 0);
        seq_num = (seq_num + 1) % MAX_SEQ_NUM; 
        if (sendto(sockfd, (void *)&synack_packet, synack_packet.getSegmentSize(), 0,
                   (struct sockaddr *)&client_addr, client_addr_length) == -1)
        {
            perror("sendto() error in server while sending SYN-ACK/SYN");
        }
        
        // receives ack packet, resending syn-ack if not received in time
        
        fd_set listening_socket;
        FD_ZERO(&listening_socket);
        FD_SET(sockfd, &listening_socket);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = INIT_RTO * 1000;
        
        if (select(sockfd + 1, &listening_socket, NULL, NULL, &timeout) > 0)
        {
            if ((nbytes = recv(sockfd, (void *)&recv_packet, sizeof(simpleHeader), 0)) == -1)
            {
                perror("recv() error in server while processing ACK");
            }
            else
            {
                ntohPacket(recv_packet);
                if(recv_packet.getACK())
                {
                    cout << "Receiving ACK packet " << recv_packet.getAckNum() << endl;
                    break;
                }
            }
            usleep(timeout.tv_usec + timeout.tv_sec * 1000000);
            continue;
        }
        else
        {
            continue;
        }
    }
    
    data_left = true;
    /*
    while (data_left)
    {
        
    }*/
}
