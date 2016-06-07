#ifndef CONSTANTS_H
#define CONSTANTS_H

// TCP packet
const int MAX_PAYLOAD = 1024;
const int F_ACK = 0x4;
const int F_SYN = 0x2;
const int F_FIN = 0x1;
const int MAX_SEGMENT_SIZE = 1032;

// Congestion/flow control
const int RECV_WINDOW = 15360;
const int MAX_PACKET_LENGTH = 1032;
const int MAX_SEQ_NUM = 30720;
const int INIT_CONG_SIZE = 1024;
const int INIT_SLOWSTART = 15360;

// Timers
const int INIT_RTO = 500; // ms
const int RTO_UBOUND = 10000000; // us -> 10 sec

// For server
const int BACKLOG = 20;


const unsigned long long ULL_MEGA = 1000000;

#endif

