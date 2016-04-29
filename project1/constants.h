#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <string>

const std::string CRLF = "\r\n";
const std::string HTTP_VERSION_10 = "1.0";
const std::string HTTP_VERSION_11 = "1.1";
const std::string HTTP_DEFAULT_VERSION = HTTP_VERSION_11;
const int HTTP_DEFAULT_PORT = 80;

//const int RECV_TIMEOUT_SECS = 3;
const int RECV_TIMEOUT_SECS = 10;
const int RECV_BUF_SIZE = 1024;

// For server
const int BACKLOG = 20; // convention
const int LISTEN_WAIT_SEC = 10;
const int LISTEN_WAIT_USEC = 0;

#endif /* CONSTANTS_H_ */
