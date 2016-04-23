#include "constants.h"
#include "HttpMessage.h"
#include "WebUtil.h"

#include <algorithm>

using namespace std;

HttpMessage::HttpMessage(const string firstLine)
    : firstLine_(firstLine)
{
    setFirstLine(firstLine);
}

// Set the first line of the HttpMessage (also extracts the HTTP version for
// this message).
void HttpMessage::setFirstLine(const string firstLine)
{
    firstLine_ = firstLine;
    httpVersion_ = getVersionFromLine(firstLine);
}

// Return the HTTP version (or "" if the first line was set incorrectly).
string HttpMessage::getHttpVersion() const
{
    return httpVersion_;
}

string HttpMessage::getFirstLine() const
{
    return firstLine_;
}

bool HttpMessage::getHeader(const string header, string& value) const
{
    string h = header;
    transform(h.begin(), h.end(), h.begin(), ::tolower);

    if (headers_.count(h) > 0)
    {
        value = headers_.at(h);
        return true;
    }
    else
    {
        return false;
    }
}

void HttpMessage::setHeader(const string header, const string value)
{
    string h = header;
    transform(h.begin(), h.end(), h.begin(), ::tolower);
    headers_[h] = value;
}

string HttpMessage::getPayload() const
{
    return payload_;
}

void HttpMessage::setPayload(const string payload)
{
    payload_ = payload;
    setHeader("Content-Length", to_string(payload.size()));
}

// Returns a string representation of this HttpMessage
string HttpMessage::toString()
{
    string result;

    result += firstLine_ + CRLF;
    for (auto it = headers_.begin(); it != headers_.end(); ++it)
        result += it->first + ": " + it->second + CRLF;

    result += CRLF;
    result += payload_;
    return result;
}
