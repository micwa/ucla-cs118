#ifndef TCPutil_h
#define TCPutil_h

int getIpv4(const string& host, int port, struct addrinfo **result)
{
    struct addrinfo hints;
    
    // Hints: IPv4 and UDP
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    return getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, result);
}

#endif
