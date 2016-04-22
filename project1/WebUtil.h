#ifndef WEB_UTIL_H_
#define WEB_UTIL_H_

#include <string>

/* HTTP RELATED */

// Return the HTTP status description corresponding to the status.
std::string HttpStatusDescription(int status);

// Return the first line of a HTTP GET request.
std::string ConstructGetRequest(std::string version, std::string path);

// Return the first line of a HTTP response.
std::string ConstructStatusLine(std::string version, int status);

/* SOCKET RELATED */

// Read a line from sockfd terminated by "term" and store it in result.
// Returns 0 if the socket returns EOF immediately, -1 if there is an error, and result.size() otherwise.
int readline(int sockfd, std::string& result, const std::string term = "\r\n");

// Send data through the socket sockfd.
// Returns true if the data was sent successfully, and false if there was an error.
bool sendAll(int sockfd, const std::string& data);

#endif /* WEB_UTIL_H_ */
