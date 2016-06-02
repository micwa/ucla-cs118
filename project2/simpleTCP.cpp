#include <cstring>

#include "simpleTCP.h"

simpleTCP::simpleTCP()
{}

simpleTCP::simpleTCP(simpleHeader header, const char *message, int size)
    : m_header(header), m_size(size)
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

void simpleTCP::setFlags(uint16_t flags)
{
    m_header.flags = flags;
}

void simpleTCP::setACK()
{
    m_header.flags |= F_ACK;
}

void simpleTCP::setSYN()
{
    m_header.flags |= F_SYN;
}

void simpleTCP::setFIN()
{
    m_header.flags |= F_FIN;
}

void simpleTCP::setMessage(const char *message, int size)
{
    memcpy(m_payload, message, size);
    m_size = size;
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

uint16_t simpleTCP::getFlags() const
{
    return m_header.flags;
}

bool simpleTCP::getACK() const
{
    return !!(m_header.flags & F_ACK);
}

bool simpleTCP::getSYN() const
{
    return !!(m_header.flags & F_SYN);
}

bool simpleTCP::getFIN() const
{
    return !!(m_header.flags & F_FIN);
}

const char *simpleTCP::getMessage() const
{
    return m_payload;
}

int simpleTCP::getHeaderSize() const
{
    return sizeof(struct simpleHeader);
}

int simpleTCP::getPayloadSize() const
{
    return m_size;
}

int simpleTCP::getSegmentSize() const
{
    return getHeaderSize() + getPayloadSize();
}
