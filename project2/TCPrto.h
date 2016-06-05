//
//  TCPrto.cpp
//  
//
//  Created by James Kang on 6/4/16.
//
//

#ifndef TCPRTO_H
#define TCPRTO_H

#include <stdio.h>
#include <algorithm>
#include <sys/time.h>
#include <constants.h>

class TCPrto
{
private:
    int srtt;
    int rto;
public:
    TCPrto();
    void rtoTimeout();
    void srtt(struct timeval rtt);
    int getSrtt();
    struct timeval getRto();
};

#endif /* TCPrto_h */
