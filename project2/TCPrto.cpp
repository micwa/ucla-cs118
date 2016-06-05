//
//  TCPrto.cpp
//  
//
//  Created by James Kang on 6/4/16.
//
//

#include "TCPrto.h"

using namespace std;

TCPrto::TCPrto()
{
    m_rto = INIT_RTO * 1000;
    m_srtt = 0;
}

void TCPrto::rtoTimeout()
{
    m_rto = min(2*m_rto, RTO_UBOUND);
}

void TCPrto::srtt(struct timeval rtt)
{
    if (m_srtt == 0) // adaptive rto
    {
        m_srtt = rtt.tv_usec + rtt.tv_sec * 1000000; // best guess with 1 data set
    }
    else
    {
        m_srtt = 4 * m_srtt / 5 + (rtt.tv_usec + rtt.tv_sec * 1000000) / 5; // alpha of rfc 793 = 0.8 (rec: [0.8,0.9])
    }
    m_rto = min(RTO_UBOUND, 3 * m_srtt / 2); // beta of rfc 793 = 1.5 (rec: [1.3,2.0])
}

int TCPrto::getSrtt()
{
    return m_srtt;
}

struct timeval TCPrto::getRto()
{
    struct timeval rtotv;
    rtotv.tv_sec = m_rto / 1000000;
    rtotv.tv_usec = m_rto % 1000000;
    return rtotv;
}