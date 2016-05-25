#include <cstring.h>
#include <sys/types.h>
#include simpleTCP.h

simpleTCP::simpleTCP(simpleHeader header, char* message)
    : m_header(header)
{
    strncpy(m_payload, message, 1024);
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

uint16_t simpleTCP::getSeqNum()
{
    return m_header.seq_num;
}

uint16_t simpleTCP::getAckNum()
{
    return m_header.ack_num;
}

uint16_t simpleTCP::getWindow()
{
    return m_header.window;
}

bool simpleTCP::getACK()
{
    return !!(flags & F_ACK);
}

bool simpleTCP::getSYN()
{
    return !!(flags & F_SYN);
}

bool simpleTCP::getFIN()
{
    return !!(flags & F_FIN);
}


