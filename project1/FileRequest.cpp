#include "FileRequest.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"

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

bool FileRequest::sendRequest(int sockfd)
{
    return sendAll(sockfd, request_->toString());
}

bool FileRequest::recvResponse(int sockfd)
{
    return false;
}
