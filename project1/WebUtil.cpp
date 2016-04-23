#include "WebUtil.h"
#include "logerr.h"

#include <sys/types.h>
#include <sys/socket.h>

using namespace std;

/* HTTP RELATED */

string ConstructGetRequest(string version, string path)
{
    return "GET " + path + " HTTP/" + version;
}

string ConstructStatusLine(string version, int status)
{
    return "HTTP/" + version + " " + to_string(status) +
           " " + HttpStatusDescription(status);
}

string GetVersionFromLine(const string& line)
{
    int i = line.find("HTTP/");
    if (i == string::npos || i + 7 >= line.size())
        return "";
    else
        return line.substr(i + 5, 3);
}

int GetStatusCodeFromStatusLine(const string& line)
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

string GetPathFromRequestLine(const string& line)
{
    int firstSpace = line.find(' ');
    int secondSpace = line.find(' ', firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1)
        return "";
    else
        return line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
}

string HttpStatusDescription(int status)
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

/* SOCKET RELATED */

int readline(int sockfd, string& result, const string term)
{
    char buf;
    int n = term.size();

    result = "";
    while (result.size() < n || result.substr(result.size() - n, n) != term)
    {
        // Recv() one byte at a time
        int received = recv(sockfd, &buf, 1, 0);
        if (received == 0)
            return 0;
        else if (received == -1)
            return -1;
        result += buf;
    }
    _DEBUG("Result: " + result);
    return result.size();
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
