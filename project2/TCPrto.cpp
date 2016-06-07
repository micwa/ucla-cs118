//
//  TCPrto.cpp
//  
//
//  Created by James Kang on 6/4/16.
//
//

#include "TCPrto.h"
#include "logerr.h"

using namespace std;

TCPrto::TCPrto()
{
    m_rto = INIT_RTO * 1000;
    m_srtt = -1;
}

void TCPrto::rtoTimeout()
{
    m_rto = min(2*m_rto, RTO_UBOUND);
}

void TCPrto::srtt(struct timeval rtt)
{
    _DEBUG("rtt.tv_usec is " + to_string(rtt.tv_usec));
    if (m_srtt == -1) // adaptive rto
    {
        m_srtt = rtt.tv_usec + rtt.tv_sec * 1000000; // best guess with 1 data set
    }
    else
    {
        m_srtt = 4.0 * m_srtt / 5 + (rtt.tv_usec + rtt.tv_sec * 1000000) / 5.0; // alpha of rfc 793 = 0.8 (rec: [0.8,0.9])
    }
    m_rto = min(RTO_UBOUND, (int)(3.0 * m_srtt / 2)); // beta of rfc 793 = 1.5 (rec: [1.3,2.0])
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
    _DEBUG("m_rto is " + to_string(m_rto));
    _DEBUG("tv_sec is " + to_string(rtotv.tv_sec));
    return rtotv;
}
