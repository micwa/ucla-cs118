#ifndef WEB_UTIL_H_
#define WEB_UTIL_H_

#include <string>
#include <vector>

/* HTTP RELATED */

// Return the first line of a HTTP GET request.
std::string ConstructGetRequest(std::string version, std::string path);

// Return the first line of a HTTP response.
std::string ConstructStatusLine(std::string version, int status);

// Returns the version after "HTTP/", or "" if there's no version.
std::string GetVersionFromLine(const std::string& line);

// Returns the status code from the status line, or -1 if there's no status code.
int GetStatusCodeFromStatusLine(const std::string& line);

// Return the path from the request line, or "" if there's no path.
std::string GetPathFromRequestLine(const std::string& line);

// Return the HTTP status description corresponding to the status.
std::string HttpStatusDescription(int status);

/* SOCKET RELATED */

// Read a line from sockfd terminated by "term" and store it in result.
// Returns 0 if the socket returns EOF, -1 if there is an error, and result.size() otherwise.
int readline(int sockfd, std::string& result, const std::string term = "\r\n");

// Read lines, using readline(), from sockfd until an empty line is encountered.
// Returns 0 if the socket returns EOF, -1 if there is an error, and the total number of bytes
// read otherwise (excluding the last empty line).
// Note: the empty line is NOT added to lines.
int readlinesUntilEmpty(int sockfd, std::vector<std::string>& lines);

// Send data through the socket sockfd.
// Returns true if the data was sent successfully, and false if there was an error.
bool sendAll(int sockfd, const std::string& data);

#endif /* WEB_UTIL_H_ */
