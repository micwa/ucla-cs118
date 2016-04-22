#include "WebUtil.h"

using namespace std;

/* HTTP RELATED */

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

string ConstructGetRequest(string version, string path)
{
    return "GET " + path + " HTTP/" + version;
}

string ConstructStatusLine(string version, int status)
{
    return "HTTP/" + version + " " + to_string(status) +
           " " + HttpStatusDescription(status);
}

/* SOCKET RELATED */

int readline(int sockfd, string& result, const string terminator)
{
    return -1;
}

bool sendall(int sockfd, const string& data)
{
    return false;
}
