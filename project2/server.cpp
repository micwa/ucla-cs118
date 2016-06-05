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

static void teardown(int sockfd, int seq_num, int ack_num, struct sockaddr *client_addr, socklen_t client_addr_length, 
                     TCPrto rtoObj)
{
    simpleTCP packet;

    while (true)
    {
        // Send FIN packet
        struct timeval timeout = rtoObj.getRto();
        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_FIN, "", 0);

        if (!sendAll(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                     client_addr, client_addr_length))
        {
            perror("sendAll() sending FIN");
        }

        // Receive ACK packet
        if (timeSocket(sockfd, &timeout) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, packet, client_addr, &client_addr_length);
            
            // Packet must have at least a header, be ACK, and have the proper sequence number
            if (nbytes < packet.getHeaderSize() || !packet.getACK() ||
                packet.getSeqNum() != ack_num)
            {
                _ERROR("Receiving ACK packet for teardown");
                continue;
            }
        }
        else
        {
            _DEBUG("Timeout receiving FIN/ACK packet");
            rtoObj.rtoTimeout();
            continue;
        }

        // ACK packet OK
        if (packet.getFIN())        // If ACK was lost, but FIN/ACK was received
            goto received_finack;
        break;
    }
    seq_num = (seq_num + 1) % MAX_SEQ_NUM;

    struct timeval timeout;         // Outside the loop, for goto statement to work
    while (true)
    {
        // Receive FIN/ACK packet
        timeout = rtoObj.getRto();

        if (timeSocket(sockfd, &timeout) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, packet, client_addr, &client_addr_length);
            
            // Packet must have at least a header, be FIN/ACK, and have the proper sequence number
            if (nbytes < packet.getHeaderSize() || !packet.getACK() || !packet.getFIN() ||
                packet.getSeqNum() != ack_num)
            {
                _ERROR("Receiving FIN/ACK packet for teardown");
                continue;
            }
        }
        else
        {
            _DEBUG("Timeout receiving FIN/ACK packet");
            rtoObj.rtoTimeout();
            continue;
        }
        
 received_finack:
        // Send ACK packet
        int new_ack = (ack_num + 1) % MAX_SEQ_NUM;
        timeout = rtoObj.getRto();
        timeout.tv_sec += timeout.tv_usec * 2 / 1000000;
        timeout.tv_usec = timeout.tv_usec * 2 % 1000000;
        
        packet = makePacket_ton(seq_num, new_ack, RECV_WINDOW, F_ACK, "", 0);
        sendAck(sockfd, client_addr, client_addr_length, packet, false);
        
        if (timeSocket(sockfd, &timeout) > 0)
            continue;
        else
            break;
    }
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
    
    int seq_num, ack_num, cong_window, ssthresh, map_size;
    simpleTCP recv_packet;
    struct sockaddr client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    bool data_left_to_read, data_left_to_send;
    bool retransmission = false;
    
    srand(time(NULL));
    seq_num = 0;//rand() % MAX_SEQ_NUM; // random initial sequence number
    cong_window = INIT_CONG_SIZE;
    ssthresh = INIT_SLOWSTART;
    
    struct timeval timeout;
    TCPrto rtoObj = TCPrto();
    
    // Receive SYN packet
    while (true)
    {
        int nbytes = recvPacket_toh(sockfd, recv_packet, &client_addr, &client_addr_length);

        if (nbytes < recv_packet.getHeaderSize() || !recv_packet.getSYN())
        {
            perror("recvfrom() SYN packet");
            continue;
        }
        break;
    }
    cout << "Receiving SYN packet" << endl;
    ack_num = (recv_packet.getSeqNum() + 1) % MAX_SEQ_NUM;
    
    while (true)
    {
        struct timeval sent_syn, recv_acksyn, diff_syn;
        
        // Send SYN/ACK packet
        cout << "Sending SYN/ACK packet " << seq_num << " " << ack_num;
        if (retransmission)
            cout << " Retransmission";
        cout << endl;
        retransmission = true;

        simpleTCP synack_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_SYN | F_ACK, "", 0);
        if (!sendAll(sockfd, (void *)&synack_packet, synack_packet.getSegmentSize(), 0,
                     (struct sockaddr *)&client_addr, client_addr_length))
        {
            perror("sendAll() sending SYN/ACK");
        }
        else
        {
            gettimeofday(&sent_syn, NULL);
        }

        // Receive ACK packet
        timeout = rtoObj.getRto();
        
        if (timeSocket(sockfd, &timeout) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, recv_packet, &client_addr, &client_addr_length);

            if (nbytes < recv_packet.getHeaderSize() || !recv_packet.getACK())
            {
                _ERROR("Receiving ACK packet for handshake");
                continue;
            }
        }
        else
        {
            _DEBUG("Timeout receiving ACK packet");
            rtoObj.rtoTimeout();
            continue;
        }

        // Good ACK packet
        cout << "Receiving ACK packet " << recv_packet.getAckNum() << endl;
        gettimeofday(&recv_acksyn, NULL);
        timersub(&recv_acksyn, &sent_syn, &diff_syn);
        rtoObj.srtt(diff_syn);
        break;
    }
    
    // Data delivery
    seq_num = (seq_num + 1) % MAX_SEQ_NUM;
    data_left_to_read = true;
    data_left_to_send = true;
    
    map_size = 0;
    
    map<unsigned long long, simpleTCP> timeSent;
    map<unsigned long long, simpleTCP>::iterator it;
    
    ifstream ifs(filename);
    
    char packet_buf[MAX_PAYLOAD];
    int payload_size = 0;
    bool already_read_buf = false; // there's probably a better way to implement this logic
    int prev_ack_num = recv_packet.getAckNum();
    int same_ack_count = 0;
    
    while (data_left_to_send)
    {
        int receiver_window = (int) recv_packet.getWindow();
        int real_window = min(cong_window, receiver_window); 
        while (data_left_to_read) // loop is exited when there is no more to read OR when too much sent for window
        {
            if (!already_read_buf)
            {
                ifs.read(packet_buf, MAX_PAYLOAD);
                if (ifs.eof())
                {
                    payload_size = ifs.gcount();
                    data_left_to_read = false;
                    ifs.close();
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
                simpleTCP data_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, packet_buf, payload_size);
                map_size += payload_size;
                struct timeval cur_time;
                gettimeofday(&cur_time, NULL);
                unsigned long long microsec_time = cur_time.tv_sec * ULL_MEGA + cur_time.tv_usec;
                timeSent[microsec_time] = data_packet;
                if (!sendAll(sockfd, (void *)&data_packet, data_packet.getSegmentSize(), 0,
                             (struct sockaddr *)&client_addr, client_addr_length))
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
        timeout = rtoObj.getRto();
        gettimeofday(&ack_time, NULL); 
        sent_time.tv_sec = timeSent.begin()->first / 1000000;
        sent_time.tv_usec = timeSent.begin()->first % 1000000;
        timersub(&ack_time, &sent_time, &time_passed); 
        timersub(&timeout, &time_passed, &timeout_left);
        
        // Receive ACK
        if (timeSocket(sockfd, &timeout_left) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, recv_packet, &client_addr, &client_addr_length);

            if (nbytes < recv_packet.getHeaderSize() || !recv_packet.getACK())
            {
                _ERROR("Receiving ACK packet");
                continue;
            }

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
        }
        else // timeout - Tahoe
        {
            _DEBUG("Timeout receiving ACK packet");
            rtoObj.rtoTimeout();

            ssthresh = cong_window / 2;
            cong_window = INIT_CONG_SIZE;
            
            int tahoe_data_sent = 0;
            for (it = timeSent.begin(); it != timeSent.end(); it++)
            {
                if (cong_window < timeSent.begin()->second.getPayloadSize() + tahoe_data_sent)
                {
                    break;
                }
                struct timeval cur_time;
                gettimeofday(&cur_time, NULL);
                unsigned long long microsec_time = cur_time.tv_sec * ULL_MEGA + cur_time.tv_usec;
                simpleTCP resent_data_packet = it->second;
                timeSent[microsec_time] = resent_data_packet;
                if (sendAll(sockfd, (void *)&resent_data_packet, resent_data_packet.getSegmentSize(), 0,
                            (struct sockaddr *)&client_addr, client_addr_length) == -1)
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

        // Check if data transfer is over
        if (!data_left_to_read && timeSent.empty())
            break;
    }
    
    // FIN teardown
    teardown(sockfd, seq_num, ack_num, &client_addr, client_addr_length, rtoObj);

    return 0;
}
