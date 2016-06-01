#ifndef CONSTANTS_H
#define CONSTANTS_H

// TCP packet
const int MAX_PAYLOAD = 1024;
const int F_ACK = 0x4;
const int F_SYN = 0x2;
const int F_FIN = 0x1;

// Congestion control/timers
const int MAX_PACKET_LENGTH = 1032;
const int MAX_SEQ_NUM = 30720;
const int INIT_CONG_SIZE = 1024;
const int INIT_SLOWSTART = 1024;
const int INIT_RTO = 500; // ms

// For server
const int BACKLOG = 20;

#endif

