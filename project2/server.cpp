#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <unordered_set>
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

namespace std
{
    template <>
    struct hash<simpleTCP>
    {
        size_t operator()(const simpleTCP& s) const
        {
            return hash<int>()(s.getSeqNum());
        }
    };
}

static void errorAndExit(const string& msg)
{
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

static void enqueuePacket(simpleTCP& packet, vector<simpleTCP>& packets, int& bytes_queued)
{
    packets.push_back(packet);
    bytes_queued += packet.getPayloadSize();
}

static void sendDataPacket(simpleTCP& packet, unordered_map<simpleTCP, struct timeval>& time_sent,
                           unordered_set<simpleTCP>& packets_sent, int& bytes_sent, int cong_window, int ssthresh,
                           int sockfd, const struct sockaddr *client_addr, socklen_t client_addr_length)
{
    cout << "Sending packet " << ntohs(packet.getSeqNum()) << " " << cong_window << " " << ssthresh;
    if (packets_sent.count(packet) > 0)
        cout << " Retransmission";
    cout << endl;

    if (!sendAll(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                 client_addr, client_addr_length))
    {                    
        perror("sendAll() error in server while sending data");
    }

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    time_sent[packet] = cur_time;
    packets_sent.insert(packet);
    bytes_sent += packet.getPayloadSize();
}

// Returns the number of packets in the vector that are ACKed by the given ack_num.
static int packetsAcked(const vector<simpleTCP>& packets, int ack_num)
{
    int npackets = 0;
    for (simpleTCP packet : packets)
    {
        ++npackets;
        if ((ntohs(packet.getSeqNum()) + packet.getPayloadSize()) % MAX_SEQ_NUM == ack_num)
            return npackets;
    }
    return 0;
}

// Acknowledge (erase) the first npackets packets from the given vector (and in the other containers).
static void ackPackets(int npackets, vector<simpleTCP>& packets, unordered_map<simpleTCP, struct timeval>& time_sent,
                       unordered_set<simpleTCP>& packets_sent, int& bytes_queued, int& bytes_sent)
{
    for (int i = 0; i < npackets; ++i)
    {
        time_sent.erase(packets[i]);
        packets_sent.erase(packets[i]);
        bytes_queued -= packets[i].getPayloadSize();
        bytes_sent -= packets[i].getPayloadSize();
    }
    packets.erase(packets.begin(), packets.begin() + npackets);
}

static void teardown(int sockfd, int seq_num, int ack_num, int cong_window, int ssthresh,
                     struct sockaddr *client_addr, socklen_t client_addr_length, TCPrto rtoObj)
{
    simpleTCP packet;
    bool retransmission = false;
    bool received_finack = false;

    while (true)
    {
        // Send FIN packet
        cout << "Sending packet " << seq_num << " " << cong_window << " " << ssthresh;
        if (retransmission)
            cout << " Retransmission";
        cout << " FIN" << endl;
        retransmission = true;

        struct timeval sent_time, recv_time, diff_time;
        gettimeofday(&sent_time, NULL);
        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_FIN|F_ACK, "", 0);

        if (!sendAll(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                     client_addr, client_addr_length))
        {
            perror("sendAll() sending FIN");
        }

        // Receive ACK packet
        struct timeval timeout = rtoObj.getRto();
        if (timeSocket(sockfd, &timeout) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, packet, client_addr, &client_addr_length);
            
            // Packet must have at least a header, be ACK, and have the proper sequence number
            // The receiver must also expect ack_num+1 (to indicate it received the FIN)
            if (nbytes < packet.getHeaderSize() || !packet.getACK() ||
                packet.getSeqNum() != ack_num || packet.getAckNum() != (seq_num + 1) % MAX_SEQ_NUM)
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
        cout << "Receiving packet " << packet.getAckNum() << endl;
        gettimeofday(&recv_time, NULL);
        timersub(&recv_time, &sent_time, &diff_time);
        rtoObj.srtt(diff_time);

        if (packet.getFIN())        // If ACK was lost, but FIN/ACK was received
            received_finack = true;
        break;
    }
    seq_num = (seq_num + 1) % MAX_SEQ_NUM;

    // Receive FIN/ACK packet
    while (!received_finack)
    {
        int nbytes = recvPacket_toh(sockfd, packet, client_addr, &client_addr_length);
        
        // Packet must have at least a header, be FIN/ACK, and have the proper sequence number
        if (nbytes < packet.getHeaderSize() || !packet.getACK() || !packet.getFIN() ||
            packet.getSeqNum() != ack_num)
        {
            _ERROR("Receiving FIN/ACK packet for teardown");
            continue;
        }
        cout << "Receiving packet " << packet.getAckNum() << endl;
        received_finack = true;
    }
    ack_num = (ack_num + 1) % MAX_SEQ_NUM;

    // Enter TIME_WAIT state; wait a maximum of RTO_UBOUND usecs
    _DEBUG("Entering TIME_WAIT state");
    struct timeval init_time, now_time, diff_time, timeout, max_timeout;
    gettimeofday(&init_time, NULL);
    gettimeofday(&now_time, NULL);
    timeout = rtoObj.getRto();
    max_timeout.tv_sec  = timeout.tv_sec * 2 + timeout.tv_usec * 2 / 1000000;
    max_timeout.tv_usec = timeout.tv_usec * 2 % 1000000;
    timersub(&now_time, &init_time, &diff_time);
    retransmission = false;

    while (timercmp(&diff_time, &max_timeout, <))
    {
        // Send ACK packet
        cout << "Sending packet " << seq_num << " " << cong_window << " " << ssthresh;
        if (retransmission)
            cout << " Retransmission";
        cout << endl;
        retransmission = true;

        packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, "", 0);
        if (!sendAll(sockfd, (void *)&packet, packet.getSegmentSize(), 0,
                     client_addr, client_addr_length))
        {
            perror("sendAll() sending ACK");
        }
        retransmission = true;
        
        timersub(&max_timeout, &diff_time, &timeout);
        if (timeSocket(sockfd, &timeout) > 0)       // ACK any further input
        {
            recvPacket_toh(sockfd, packet, client_addr, &client_addr_length);
            gettimeofday(&now_time, NULL);
            timersub(&now_time, &init_time, &diff_time);
            continue;
        }
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
    
    int seq_num, ack_num, bytes_sent, bytes_queued;
    double cong_window, ssthresh;
    simpleTCP recv_packet;
    struct sockaddr client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    bool data_left_to_read, data_left_to_send;
    bool retransmission = false;
    
    srand(time(NULL));
    seq_num = 0;//rand() % MAX_SEQ_NUM; // random initial sequence number
    cong_window = INIT_CONG_SIZE;
    ssthresh = INIT_SLOWSTART;
    
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
    cout << "Receiving packet " << recv_packet.getAckNum() << endl;
    ack_num = (recv_packet.getSeqNum() + 1) % MAX_SEQ_NUM;
    
    while (true)
    {
        struct timeval sent_syn, recv_acksyn, diff_syn;
        
        // Send SYN/ACK packet
        cout << "Sending packet " << seq_num << " " << (int)cong_window << " " << (int)ssthresh;
        if (retransmission)
            cout << " Retransmission";
        cout << " SYN" << endl;
        retransmission = true;

        simpleTCP synack_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_SYN | F_ACK, "", 0);
        if (!sendAll(sockfd, (void *)&synack_packet, synack_packet.getSegmentSize(), 0,
                     &client_addr, client_addr_length))
        {
            perror("sendAll() sending SYN/ACK");
        }
        else
        {
            gettimeofday(&sent_syn, NULL);
        }

        // Receive ACK packet
        struct timeval timeout = rtoObj.getRto();
        
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
    
    bytes_queued = 0;
    bytes_sent = 0;
    
    vector<simpleTCP> unacked_packets;
    unordered_map<simpleTCP, struct timeval> time_sent;
    unordered_set<simpleTCP> packets_sent;
    
    ifstream ifs(filename);
    char packet_buf[MAX_PAYLOAD];
    int payload_size = 0;
    bool already_read_buf = false; // there's probably a better way to implement this logic
    
    int prev_ack = -1;
    int dup_ack_count = 0;
    
    while (data_left_to_send)
    {
        int receiver_window = (int) recv_packet.getWindow();
        int real_window = min((int) cong_window, receiver_window); 
        // Enqueue data packet
        // Loop is exited when there is no more to read OR when too much enqueued for window
        // 'Queue' is actually a vector
        while (data_left_to_read && bytes_queued < real_window) 
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
            
            if (real_window < payload_size + bytes_queued)
            {
                already_read_buf = true;
                break;
            }
            else
            {
                simpleTCP data_packet = makePacket_ton(seq_num, ack_num, RECV_WINDOW, F_ACK, packet_buf, payload_size);
                enqueuePacket(data_packet, unacked_packets, bytes_queued);
                seq_num = (seq_num + payload_size) % MAX_SEQ_NUM;
            }
        }

        // Send data packets (at most real_window bytes)
        if (unacked_packets.size() == 0)
        {
            _ERROR("Somehow, no packets are in the queue");
            continue;
        }
        for (int i = 0; i != unacked_packets.size(); ++i)
        {
            simpleTCP& packet = unacked_packets[i];

            if (time_sent.count(packet) > 0)
                continue;
            if (bytes_sent + packet.getPayloadSize() > real_window)
                break;

            sendDataPacket(packet, time_sent, packets_sent, bytes_sent, (int) cong_window,
                           ssthresh, sockfd, &client_addr, client_addr_length);
            if (packet.getSeqNum() == prev_ack)
                dup_ack_count = 0;
        }

        // Note that time has already passed between sending the packet and now
        struct timeval ack_time, sent_time, time_passed, timeout, timeout_left;
        timeout = rtoObj.getRto();
        gettimeofday(&ack_time, NULL); 
        sent_time = time_sent[unacked_packets.front()];
        timersub(&ack_time, &sent_time, &time_passed); 
        timersub(&timeout, &time_passed, &timeout_left);
        
        // Receive ACK
        if (timeSocket(sockfd, &timeout_left) > 0)
        {
            int nbytes = recvPacket_toh(sockfd, recv_packet, &client_addr, &client_addr_length);
            int received_ack = recv_packet.getAckNum();

            if (nbytes < recv_packet.getHeaderSize() || !recv_packet.getACK())
            {
                _ERROR("Receiving ACK packet");
                continue;
            }

            cout << "Receiving packet " << received_ack << endl;
            int npackets = packetsAcked(unacked_packets, received_ack);
            ackPackets(npackets, unacked_packets, time_sent, packets_sent,
                       bytes_queued, bytes_sent);

            // Only do stuff if the ACK was valid
            if (npackets > 0)
            {
                // Congestion control
                if (cong_window < ssthresh) // slow start
                    cong_window += MAX_PAYLOAD;
                else // Congestion Avoidance
                    cong_window += MAX_PAYLOAD * MAX_PAYLOAD  / cong_window;

                prev_ack = received_ack;
                dup_ack_count = 0;
                rtoObj.srtt(time_passed);
                _DEBUG(to_string(npackets) + " ACKed");
            }
            else
            {
                ++dup_ack_count;
            }
        }
        else // timeout - Tahoe
        {
            _DEBUG("Timeout receiving ACK packet");
            time_sent.clear();
            bytes_sent = 0;

            rtoObj.rtoTimeout();
            ssthresh = max((int)cong_window / 2, INIT_CONG_SIZE);
            cong_window = INIT_CONG_SIZE;
        }

        if (dup_ack_count == 3) // fast retransmit and revert to slow start
        {
            _DEBUG("Fast retransmit after 3 duplicate ACKs");
            time_sent.clear();
            bytes_sent = 0;

            rtoObj.rtoTimeout();
            ssthresh = max((int)cong_window / 2, INIT_CONG_SIZE);
            cong_window = INIT_CONG_SIZE;
        }
        
        // Check if data transfer is over
        if (!data_left_to_read && !already_read_buf && unacked_packets.empty())
            break;
    }
    
    // FIN teardown
    teardown(sockfd, seq_num, ack_num, cong_window, ssthresh,
             &client_addr, client_addr_length, rtoObj);

    return 0;
}
