#ifndef SIMPLETCP_H
#define SIMPLETCP_H

#include 'constants.h'

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
    int m_payload_size;
public:
    simpleTCP(simpleHeader header, const char *message, int size);
    void setSeqNum(uint16_t seqnum);
    void setAckNum(uint16_t acknum);
    void setWindow(uint16_t windownum);
    void setACK();
    void setSYN();
    void setFIN();
    void setMessage(const char *message, int size)
    uint16_t getSeqNum() const;
    uint16_t getAckNum() const;
    uint16_t getWindow() const;
    bool getACK() const;
    bool getSYN() const;
    bool getFIN() const;
    char *getMessage() const;
    int getPayloadSize() const;
};

#endif SIMPLETCP_H
