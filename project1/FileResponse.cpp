#include "FileResponse.h"
#include "WebUtil.h"

#include <iostream>
#include <fstream>

using namespace std;

FileResponse::FileResponse()
    : requestOk_(false),
      request_(nullptr),
      response_(nullptr)
{}

FileResponse::~FileResponse()
{
    delete request_;
    delete response_;
}

bool FileResponse::recvRequest(int sockfd)
{
    return false;
}

bool FileResponse::sendResponse(int sockfd, const string& baseDir)
{
    if (!request_)
        return false;

    string httpVersion = request_->getHttpVersion();
    int status;
    string payload;

    if (requestOk_)     // Request was received and in the correct format
    {
        // Parse request for payload path (file path)
        string line = request_->getFirstLine();
        int firstSpace = line.find(' ');
        int secondSpace = line.find(' ', firstSpace + 1);
        string filepath = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);

        // Read file, and 404 if not found (or I/O error)
        ifstream ifs(baseDir + filepath);
        if (ifs.fail())
            status = 404;
        else
        {
            payload = string(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());
            if (ifs.fail())
                status = 404;
            else
                status = 200;
        }
        ifs.close();
    }
    else                // Request was received, but malformed
    {
        status = 400;
    }
    
    // Delete old response and construct new one
    delete response_;
    response_ = new HttpResponse(httpVersion, status, payload);

    return sendAll(sockfd, response_->toString());
}
