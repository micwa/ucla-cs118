//
//  TCPrto.cpp
//  
//
//  Created by James Kang on 6/4/16.
//
//

#include "TCPrto.h"

TCPrto::TCPrto()
{
    rto = INIT_RTO * 1000;
    srtt = 0;
}

void TCPrto::rtoTimeout()
{
    rto = min(2*rto, RTO_UBOUND);
}

void srtt(struct timeval rtt)
{
    if (srtt == 0) // adaptive rto
    {
        srtt = rtt.tv_usec + rtt.tv_sec * 1000000; // best guess with 1 data set
    }
    else
    {
        srtt = 4 * srtt / 5 + (rtt.tv_usec + rtt.tv_sec * 1000000) / 5; // alpha of rfc 793 = 0.8 (rec: [0.8,0.9])
    }
    rto = min(RTO_UBOUND, 3 * srtt / 2); // beta of rfc 793 = 1.5 (rec: [1.3,2.0])
}

int getSrtt()
{
    return srtt;
}

struct timeval getRto()
{
    struct timeval rtotv;
    rtotv.tv_sec = rto / 1000000;
    rtotv.tv_usec = rto % 1000000;
    return rtotv;
}