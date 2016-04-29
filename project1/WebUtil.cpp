#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

#include <algorithm>
#include <cstring>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

/* HTTP RELATED */

static int skipWhitespace(const string& line, int start);
static int skipWhitespaceBackwards(const string& line, int start);

bool checkRequestLineValid(const string& line)
{
    // Must start with "GET" (case sensitive)
    if (line.substr(0, 3) != "GET")
        return false;

    // Must have two spaces (tabs don't count); be lenient with multiple spaces
    int firstSpace = line.find(' ');
    if (firstSpace == string::npos)
        return false;
    int secondSpace = line.find(' ', skipWhitespace(line, firstSpace + 1));
    if (secondSpace == string::npos)
        return false;
    int thirdSpace = line.find(' ', skipWhitespace(line, secondSpace + 1));
    if (thirdSpace != string::npos)
        return false;

    // After second space, must be "HTTP/<version>" (case sensitive)
    int i = skipWhitespace(line, secondSpace + 1);
    if (line.substr(i, 5) != "HTTP/" || i + 5 >= line.size())
        return false;
    return true;
}

string constructGetRequest(string version, string path)
{
    return "GET " + path + " HTTP/" + version;
}

string constructStatusLine(string version, int status)
{
    return "HTTP/" + version + " " + to_string(status) +
           " " + httpStatusDescription(status);
}

string getVersionFromLine(const string& line)
{
    const string VERSION_START = "HTTP/";   // Case-sensitive

    int i = line.find(VERSION_START);
    if (i == string::npos || i + 7 >= line.size())
        return "";
    else
    {
        // Version must be 1.0 or 1.1
        string version = line.substr(i + VERSION_START.size(), 3);
        if (version == HTTP_VERSION_11 || version == HTTP_VERSION_10)
            return version;
        return "";
    }
}

int getStatusCodeFromStatusLine(const string& line)
{
    int firstSpace = line.find(' ');
    int secondSpace = line.find(' ', firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1)
        return -1;
    else
    {
        string status = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        int i;
        try {
            i = stoi(status);
        }
        catch (...) {
            i = -1;
        }
        return i;
    }
}

string getPathFromRequestLine(const string& line)
{
    int firstSpace = line.find(' ');
    if (firstSpace == -1)
        return "";
    firstSpace = skipWhitespace(line, firstSpace + 1) - 1;
    int secondSpace = line.find(' ', firstSpace + 1);

    if (secondSpace == -1)
        return "";
    else
        return line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
}

string httpStatusDescription(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 400:
        return "Bad request";
    case 404:
        return "Not found";
    default:
        return "";
    }
}

// Populates the HttpMessage with the given headerLines.
// Returns false if any of the header lines are malformed, and true otherwise.
static bool populateHeaders(HttpMessage *msg, const vector<string>& headerLines)
{
    for (string line : headerLines)
    {
         string header, value;
         if (splitHeaderLine(line, header, value))
             msg->setHeader(header, value);
         else
            return false;
    }
    return true;
}

HttpRequest *makeHttpRequest(string httpVersion, string host, string path,
                             const vector<string>& headerLines)
{
    HttpRequest *request = new HttpRequest(httpVersion, host, path);

    if (populateHeaders(request, headerLines))
        return request;
    else
    {
        delete request;
        return nullptr;
    }
}

HttpResponse *makeHttpResponse(string httpVersion, int status, string payload,
                               const vector<string>& headerLines)
{
    HttpResponse *response = new HttpResponse(httpVersion, status, payload);

    if (populateHeaders(response, headerLines))
        return response;
    else
    {
        delete response;
        return nullptr;
    }
}

bool parseUrl(const string& url, string& host, int& port, string& path)
{
    const string PROTOCOL_SEPARATOR = "://";
    const string ROOT = "/";
    const string PORT_SEPARATOR = ":";
    string s = url;

    // If PROTOCOL_SEPARATOR exists, remove it
    int i = s.find(PROTOCOL_SEPARATOR);
    if (i != string::npos)
        s = s.substr(i + PROTOCOL_SEPARATOR.size());
    if (s.size() == 0)  // Host can not be empty
        return false;

    // Everything after (and including) ROOT is part of path; if no path, path = ROOT
    i = s.find(ROOT);
    if (i != string::npos)
    {
        path = s.substr(i);
        s = s.substr(0, i);
        if (s.size() == 0)
            return false;
    }
    else
        path = ROOT;

    // Parse port, with try/catch
    i = s.find(PORT_SEPARATOR);
    if (i != string::npos)
    {
        if (s.find(PORT_SEPARATOR, i + 1) != string::npos)     // Only one colon allowed
            return false;
        string portString = s.substr(i + 1);
        s = s.substr(0, i);

        if (s.size() == 0)
            return false;
        if (portString.size() == 0)
            port = HTTP_DEFAULT_PORT;
        else
        {
            try {
                port = stoi(portString);
            }
            catch (...) {
                return false;
            }
        }
    }
    else
        port = HTTP_DEFAULT_PORT;

    host = s;
    return true;
}

// Returns the first non-whitespace character, starting from start.
static int skipWhitespace(const string& line, int start)
{
    while (start != line.size() && (line[start] == ' ' || line[start] == '\t'))
        ++start;
    return start;
}

// Returns the first non-whitespace character going backwards, starting from start.
static int skipWhitespaceBackwards(const string& line, int start)
{
    while (start >= 0 && (line[start] == ' ' || line[start] == '\t'))
        --start;
    return start;
}

bool splitHeaderLine(const string& headerLine, string& header, string& value)
{
    if (headerLine.size() < 3 || headerLine[0] == ' ')
        return false;

    // Remove CRLF if present
    string line = headerLine;
    if (line.substr(line.size() - CRLF.size(), CRLF.size()) == CRLF)
        line = line.substr(0, line.size() - CRLF.size());
    
    int colon = line.find(':');
    int i;
    if (colon == string::npos)
        return false;

    // Skip whitespace and set header
    i = skipWhitespaceBackwards(line, colon - 1);
    if (i >= 0)
        header = line.substr(0, i + 1);

    // Set value and trim whitespace; allow empty values
    int space = skipWhitespace(line, colon + 1);
    if (space != line.size())
        value = line.substr(space);
    else
        value = "";

    space = skipWhitespaceBackwards(value, value.size() - 1);
    value = value.substr(0, space + 1);

    return true;
}

/* SOCKET RELATED */

int getIpv4(const string& host, int port, struct addrinfo **result)
{
    struct addrinfo hints;

    // Hints: IPv4 and TCP
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    return getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, result);
}

int readline(int sockfd, string& result, const string term)
{
    char buf;
    int n = term.size();

    result = "";
    while (result.size() < n || result.substr(result.size() - n, n) != term)
    {
        // Recv() one byte at a time
        int res = recvWithTimeout(sockfd, &buf, 1, RECV_TIMEOUT_SECS);
        if (res == 0 || res == -1)
            return res;
        result += buf;
    }
    //_DEBUG("Read line: " + result.substr(0, result.size() - term.size()));
    return result.size();
}

int readlinesUntilEmpty(int sockfd, vector<string>& lines)
{
    string line;
    int total = 0;

    lines.clear();
    while (true)
    {
        int res = readline(sockfd, line);
        if (res == 0 || res == -1)           // Propagate EOF and error from readline()
            return res;
        if (line == CRLF)
            break;

        lines.push_back(line);
        total += res;
    }
    _DEBUG("Total lines read: " + to_string(lines.size()));
    return total;
}

bool recvAll(int sockfd, std::string& result, int nbytes)
{
    char buf[RECV_BUF_SIZE + 1];
    int total = 0;
    int bytesLeft = nbytes;

    result = "";
    while (total < nbytes)
    {
        int n = min(RECV_BUF_SIZE, bytesLeft);
        int received = recvWithTimeout(sockfd, buf, n, RECV_TIMEOUT_SECS);
        //_DEBUG("Received: " + to_string(received) + " bytes");
        if (received == 0 || received == -1)
            return false;

        buf[received] = '\0';
        result += buf;
        total += received;
        bytesLeft -= received;
    }
    _DEBUG("Total received: " + to_string(total) + " bytes");
    return true;
}

int recvWithTimeout(int sockfd, char *buf, int nbytes, int timeout)
{
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(sockfd, &readset);

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // Wait for recv() or timeout
    int res = select(sockfd + 1, &readset, NULL, NULL, &tv);
    if (res == 0 || res == -1)
    {
        _ERROR("recv() timeout/error: " + to_string(res));
        return 0;
    }

    // Return recv()
    return recv(sockfd, buf, nbytes, 0);
}

bool sendAll(int sockfd, const string& data)
{
    int total = 0;
    int bytesLeft = data.size();
    const char *buf = data.c_str();

    while (total < data.size())
    {
        int sent = send(sockfd, buf + total, min(bytesLeft, MAX_SENT_BYTES), 0);
        _DEBUG("Sent: " + to_string(sent) + " bytes");
        if (sent == -1)
            return false;
        total += sent;
        bytesLeft -= sent;
    }
    _DEBUG("Total sent: " + to_string(total) + " bytes");
    return true;
}
