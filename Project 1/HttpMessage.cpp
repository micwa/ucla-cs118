#include "HttpMessage.h"

using namespace std;

HttpMessage::HttpMessage(const string firstLine)
    : firstLine_(firstLine)
{}

// Return the HTTP version (or "" if the first line was set incorrectly).
string HttpMessage::getHttpVersion() const
{
    return httpVersion_;
}

string HttpMessage::getFirstLine() const
{
    return firstLine_;
}

// Set the first line of the HttpMessage (also extracts the HTTP version for
// this message).
void HttpMessage::setFirstLine(const string firstLine)
{
    firstLine_ = firstLine;
    int i = firstLine_.find("HTTP/");
    if (i == string::npos || i + 7 >= firstLine_.size())
        httpVersion_ = "";
    else
        httpVersion_ = firstLine_.substr(i + 5, 3);
}

string HttpMessage::getHeader(const string header) const
{
    if (headers_.count(header) > 0)
        return headers_.at(header);
    else
        return "";
}

void HttpMessage::setHeader(const string header, const string value)
{
    headers_[header] = value;
}

string HttpMessage::getPayload() const
{
    return payload_;
}

void HttpMessage::setPayload(const string payload)
{
    payload_ = payload;
}

// Returns a string representation of this HttpMessage
string HttpMessage::toString()
{
    const string CRLF = "\r\n";
    string result;

    result += firstLine_ + CRLF;
    for (auto it = headers_.begin(); it != headers_.end(); ++it)
        result += it->first + ": " + it->second + CRLF;

    result += CRLF;
    result += payload_;
    return result;
}