#ifndef SIMPLETCP_H
#define SIMPLETCP_H

const int MAX_PAYLOAD = 1024;
const int F_ACK = 0x4;
const int F_SYN = 0x2;
const int F_FIN = 0x1;

struct simpleHeader
{
public:
    uint16_t seq_num;
    uint16_t ack_num;
    uint16_t window;
    uint16_t flags; // 0000000000000ASF
};

class simpleTCP
{
private:
    simpleHeader m_header;
    char m_payload[MAX_PAYLOAD];
public:
    simpleTCP(simpleHeader header, char* message);
    void setSeqNum(uint16_t seqnum);
    void setAckNum(uint16_t acknum);
    void setWindow(uint16_t windownum);
    void setACK();
    void setSYN();
    void setFIN();
    uint16_t getSeqNum();
    uint16_t getAckNum();
    uint16_t getWindow();
    bool getACK();
    bool getSYN();
    bool getFIN();
};

#endif SIMPLETCP_H
