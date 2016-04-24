#include "FileRequest.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

using namespace std;

FileRequest::FileRequest(string httpVersion, string host, string path)
    : request_(nullptr),
      response_(nullptr)
{
    request_ = new HttpRequest(httpVersion, host, path);
}

FileRequest::~FileRequest()
{
    delete request_;
    delete response_;
}

HttpResponse *FileRequest::getResponse() const
{
    return response_;
}

bool FileRequest::sendRequest(int sockfd)
{
    return sendAll(sockfd, request_->toString());
}

// To be well-formed, the response must have:
//  - HTTP version and status
//  - well-formed header lines
//  - if status == 200, must have content-length
bool FileRequest::recvResponse(int sockfd)
{
    // Receive the first line
    string firstLine;
    int res = readline(sockfd, firstLine);
    if (res == 0 || res == -1)
        return false;

    string httpVersion = getVersionFromLine(firstLine);   
    int status = getStatusCodeFromStatusLine(firstLine);
    if (httpVersion == "" || status == -1)
        return false;

    // Receive subsequent lines
    vector<string> lines;
    res = readlinesUntilEmpty(sockfd, lines);
    if ((res == 0 && status == 200) || res == -1)
        return false;

    // Create HttpResponse from lines read
    delete response_;
    response_ = makeHttpResponse(httpVersion, status, "", lines);
    if (!response_)
        return false;

    // Receive payload only if "content-length" header present
    string size;
    string payload;
    if (response_->getHeader("content-length", size))
    {
        if (!recvAll(sockfd, payload, stoi(size)))
            return false;
        response_->setPayload(payload);
    }
    else if (status == 200)
        return false;

    _DEBUG("HttpResponse received successfully");
    return true;
}
