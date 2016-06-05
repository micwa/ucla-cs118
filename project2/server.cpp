#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>
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

static void teardown(int sockfd, struct sockaddr *server_addr, socklen_t server_addr_length)
{

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
    
    int seq_num, ack_num, cong_window, ssthresh, nbytes, map_size;
    simpleTCP recv_packet;
    struct sockaddr client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    bool init_syn, data_left_to_read, data_left_to_send;
    
    srand(time(NULL));
    seq_num = 0;//rand() % MAX_SEQ_NUM; // random initial sequence number
    cong_window = INIT_CONG_SIZE;
    ssthresh = INIT_SLOWSTART;
    init_syn = false;
    
    struct timeval timeout;
    TCPrto rtoObj = TCPrto();
    
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
            ack_num = (recv_packet.getSeqNum() + 1) % MAX_SEQ_NUM;
        }
    }
    
    while (true)
    {
        // sends syn-ack packet
        simpleTCP synack_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_SYN | F_ACK, "", 0);
        if (sendAll(sockfd, (void *)&synack_packet, synack_packet.getSegmentSize(), 0,
                   (struct sockaddr *)&client_addr, client_addr_length) == -1)
        {
            perror("sendAll() error in server while sending SYN-ACK/SYN");
        }
        else
        {
            gettimeofday(sent_syn, NULL);
        }
        // receives ack packet, resending syn-ack if not received in time
        
        struct timeval sent_syn, recv_acksyn, diff_syn;
        timeout = rtoObj.getRto();
        
        if (timeSocket(sockfd, &timeout) > 0)
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
                    gettimeofday(recv_acksyn, NULL);
                    timersub(recv_acksyn, sent_syn, diff_syn);
                    rtoObj.srtt(diff_syn);
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
    
    seq_num = (seq_num + 1) % MAX_SEQ_NUM;
    
    // Data delivery
    
    data_left_to_read = true;
    data_left_to_send = true;
    
    map_size = 0;
    
    map<unsigned long long, simpleTCP> timeSent;
    map<unsigned long long, simpleTCP>::iterator it;
    
    ofstream ofs;
    ofs.open(filename, fstream::out);
    
    char packet_buf[MAX_PAYLOAD];
    int payload_size;
    bool already_read_buf = false; // there's probably a better way to implement this logic
    int prev_ack_num = recv_packet.getAckNum();
    int same_ack_count = 0;
    
    while (data_left_to_send)
    {
        int real_window = min(cong_window, recv_packet.getWindow()); 
        while (data_left_to_read) // loop is exited when there is no more to read OR when too much sent for window
        {
            if (!already_read_buf)
            {
                ofs.read(packet_buf, MAX_PAYLOAD);
                if (ofs.eof())
                {
                    payload_size = ofs.gcount() % MAX_PAYLOAD;
                    data_left_to_read = false;
                    ofs.close();
                }
                else
                {
                    payload_size = MAX_PAYLOAD;
                }
            }
            else
            {
                already_read_buf = false;
            }
            
            if (real_window < payload_size + map_size)
            {
                already_read_buf = true;
                break;
            }
            else
            {
                simpleTCP data_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, 0, packet_buf, payload_size);
                map_size += payload_size;
                struct timeval cur_time;
                gettimeofday(cur_time, NULL);
                unsigned long long microsec_time = cur_time.tv_sec * ULL_MEGA + cur_time.tv_usec;
                timeSent[microsec_time] = data_packet;
                if (sendAll(sockfd, (void *)&data_packet, data_packet.getSegmentSize(), 0,
                       client_addr, client_addr_length)) == -1)
                {                    
                    perror("sendAll() error in server while sending data");
                }
                else
                {
                    cout << "Sending data packet " << seq_num << " " << cong_window << " " << ssthresh << endl;
                }
                seq_num = (seq_num + payload_size) % MAX_SEQ_NUM;
            }
        }
        
        // congestion control
        if (cong_window < 2 * ssthresh)
        {
            cong_window = cong_window * 2;
        }
        else if (cong_window < ssthresh)
        {
            cong_window = ssthresh;
        }
        
        // note that time has already passed between sending the packet and now
        struct timeval ack_time, sent_time, time_passed, timeout_left;
        gettimeofday(ack_time, NULL); 
        sent_time.tv_sec = timeSent.begin()->first / 1000000;
        sent_time.tv_usec = timeSent.begin()->first % 1000000;
        timevalsub(ack_time, sent_time, time_passed); 
        timevalsub(timeout, time_passed, timeout_left);
        
        // Receives ACK
        if (timeSocket(sockfd, &timeout_left) > 0)
        {
            if ((nbytes = recv(sockfd, (void *)&recv_packet, MAX_SEGMENT_SIZE, 0)) == -1)
            {
                perror("recv() error in server while processing ACK");
            }
            else // Received ACK packet
            {
                ntohPacket(recv_packet);
                if(recv_packet.getACK())
                {
                    cout << "Receiving ACK packet " << recv_packet.getAckNum() << endl;
                    if (prev_ack_num == recv_packet.getAckNum())
                    {
                        same_ack_count++;
                    }
                    else
                    {
                        prev_ack_num = recv_packet.getAckNum();
                        same_ack_count = 0;
                    }
                    if (recv_packet.getAckNum() == timeSent.begin()->second.getSeqNum() + timeSent.begin()->second.getPayloadSize())
                    {
                        map_size -= timeSent.begin()->second.getPayloadSize();
                        timeSent.erase(timeSent.begin()); // actually the right ack, erase from map
                        
                        rtoObj.srtt(time_passed);
                        timeout = rtoObj.getRto();
                        
                        if (cong_window >= ssthresh) // Congestion control
                        {
                            cong_window++;
                        }
                    }
                    // a wrong ack packet can probably be safely ignored
                }
            }
        }
        else // timeout - Tahoe
        {
            ssthresh = cong_window / 2;
            cong_window = INIT_CONG_SIZE;
            
            rtoObj.rtoTimeout();
            
            int tahoe_data_sent = 0;
            for (it = timeSent.begin(); it != timeSent.end(); it++)
            {
                if (cong_window < timeSent.begin()->second.getPayloadSize() + tahoe_data_sent)
                {
                    break;
                }
                struct timeval cur_time;
                gettimeofday(cur_time, NULL);
                unsigned long long microsec_time = cur_time.tv_sec * ULL_MEGA + cur_time.tv_usec;
                simpleTCP resent_data_packet = it->second;
                timeSent[microsec_time] = resent_data_packet;
                if (sendAll(sockfd, (void *)&resent_data_packet, resent_data_packet.getSegmentSize(), 0,
                            client_addr, client_addr_length)) == -1)
                {                    
                    perror("sendAll() error in server while sending data");
                }
                else
                {
                    cout << "Sending data packet " << resent_data_packet.getSeqNum() << " " << cong_window << " " << ssthresh << " Retransmission" << endl;
                }
                tahoe_data_sent += timeSent.begin()->second.getPayloadSize();
                timeSent.erase(it);
            }
        }
        
        
        /*
        if (same_ack_count == 3) // three dup acks - Reno
        {
            
        }
        */
        
        
        if (!data_left_to_read && timeSent.empty())
        {
            break; // done with data transfer
        }
    }
    
    teardown();
    // FIN teardown
}













