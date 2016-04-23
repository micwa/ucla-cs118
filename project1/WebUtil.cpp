#include "HttpRequest.h"
#include "WebUtil.h"
#include "logerr.h"

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;

/* HTTP RELATED */

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
    int i = line.find("HTTP/");
    if (i == string::npos || i + 7 >= line.size())
        return "";
    else
        return line.substr(i + 5, 3);
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
        return stoi(status);
    }
}

string getPathFromRequestLine(const string& line)
{
    int firstSpace = line.find(' ');
    int secondSpace = line.find(' ', firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1)
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

HttpRequest *makeHttpRequest(string httpVersion, string host, string path,
                             const vector<string>& headerLines)
{
    HttpRequest *request = new HttpRequest(httpVersion, host, path);

    for (string line : headerLines)
    {
         string header, value;
         if (splitHeaderLine(line, header, value))
             request->setHeader(header, value);
         else
         {
             delete request;
             return nullptr;
         }
    }
    return request;
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
    _DEBUG("Read line: " + result);
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
        _ERROR("recv() timed out");
        return -1;
    }

    // Return recv(), basically; if for some reason sockfd is not set, return error
    if (FD_ISSET(sockfd, &readset))
        return recv(sockfd, buf, nbytes, 0);
    else
    {
        _ERROR("select() erroneously unblocked");
        return -1;
    }
}

bool sendAll(int sockfd, const string& data)
{
    int total = 0;
    int bytesLeft = data.size();
    const char *buf = data.c_str();

    while (total < data.size())
    {
        int sent = send(sockfd, buf + total, bytesLeft, 0);
        _DEBUG("Sent: " + to_string(sent) + " bytes");
        if (sent == -1)
            return false;
        total += sent;
        bytesLeft -= sent;
    }
    _DEBUG("Total sent: " + to_string(total) + " bytes");
    return true;
}
