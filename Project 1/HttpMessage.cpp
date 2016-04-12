#include "HttpMessage.h"

using namespace std;

HttpMessage::HttpMessage(const string firstLine)
    : firstLine_(firstLine)
{}

string HttpMessage::getHttpVersion() const
{
    // parse header line
}

string HttpMessage::getHeader(const string header)
{
    if (headers_.count(header) > 0)
        return headers_[header];
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