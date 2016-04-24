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
    _DEBUG("Received first line: " + firstLine.substr(0, firstLine.size() - CRLF.size()));

    // Receive subsequent lines
    vector<string> lines;
    res = readlinesUntilEmpty(sockfd, lines);
    if (res == 0 || res == -1)
        return false;

    // Create HttpResponse from lines read
    delete response_;
    response_ = makeHttpResponse(httpVersion, status, "", lines);

    // Receive payload
    string size;
    string payload;
    if (!response_ || !response_->getHeader("content-length", size))
        return false;
    if (!recvAll(sockfd, payload, stoi(size)))
        return false;

    // Set payload; make the user save the file
    response_->setPayload(payload);

    return true;
}
