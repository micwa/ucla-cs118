#include <cstring.h>
#include <sys/types.h>
#include simpleTCP.h
#include constants.h

simpleTCP::simpleTCP(simpleHeader header, const char *message, int size)
    : m_header(header), m_payload_size(size)
{
    memcpy(m_payload, message, size);
}

void simpleTCP::setSeqNum(uint16_t seqnum)
{
    m_header.seq_num = seqnum;
}

void simpleTCP::setAckNum(uint16_t acknum)
{
    m_header.ack_num = acknum;
}

void simpleTCP::setWindow(uint16_t windownum)
{
    m_header.window = windownum;
}

void simpleTCP::setACK()
{
    flags |= F_ACK;
}

void simpleTCP::setSYN()
{
    flags |= F_SYN;
}

void simpleTCP::setFIN()
{
    flags |= F_FIN;
}

void simpleTCP::setMessage(const char *message, int size)
{
    memcpy(m_payload, message, size);
}

uint16_t simpleTCP::getSeqNum() const
{
    return m_header.seq_num;
}

uint16_t simpleTCP::getAckNum() const
{
    return m_header.ack_num;
}

uint16_t simpleTCP::getWindow() const
{
    return m_header.window;
}

bool simpleTCP::getACK() const
{
    return !!(flags & F_ACK);
}

bool simpleTCP::getSYN() const
{
    return !!(flags & F_SYN);
}

bool simpleTCP::getFIN() const
{
    return !!(flags & F_FIN);
}

char* simpleTCP::getMessage() const
{
    char message[1024];
    memcpy(message, m_payload, m_size);
    return message;
}

int simpleTCP::getSize() const
{
    return m_size;
}

