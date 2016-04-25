#include "FileResponse.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

FileResponse::FileResponse()
    : httpVersion_(""),
      request_(nullptr),
      response_(nullptr)
{}

FileResponse::~FileResponse()
{
    delete request_;
    delete response_;
}

HttpRequest *FileResponse::getRequest() const
{
    return request_;
}

int FileResponse::recvRequest(int sockfd)
{
    // Receive the first line
    string firstLine;
    int res = readline(sockfd, firstLine);
    
    // If there's no HTTP version, just use the default.
    // An HttpRequest will only be created if the line is valid anyway.
    if (res != 0 && !checkRequestLineValid(firstLine))
        res = -1;
    httpVersion_ = getVersionFromLine(firstLine);   
    if (httpVersion_ == "")
        httpVersion_ = HTTP_DEFAULT_VERSION;

    // Receive subsequent lines
    vector<string> lines;
    int res2 = readlinesUntilEmpty(sockfd, lines);
    if (res == 0 || res == -1)
        return res;
    else if (res2 == 0 || res2 == -1)
        return res2;

    // Create an HttpRequest with a dummy host (host will be set during makeHttpRequest())
    string path = getPathFromRequestLine(firstLine);
    delete request_;
    request_ = makeHttpRequest(httpVersion_, "", path, lines);

    // Make sure all headers are NOT malformed and "host" header is present
    string host;
    if (!request_)
        return -1;
    if (!request_->getHeader("host", host))
    {
        delete request_;
        request_ = nullptr;
        return -1;
    }
    _DEBUG("Successfully received HTTP request");

    // Don't bother with payloads, since we assume GET requests only

    return 1;
}

bool FileResponse::sendResponse(int sockfd, const string& baseDir)
{
    if (httpVersion_ == "")
        return false;

    string httpVersion = httpVersion_;
    int status;
    string payload;

    if (request_)     // Request was received and in the correct format
    {
        // Parse request for payload path (file path)
        string line = request_->getFirstLine();
        string filepath = baseDir + getPathFromRequestLine(line);

        // Read file, and 404 if not found (or I/O error)
        _DEBUG("Request OK, reading from: " + filepath);
        ifstream ifs(filepath);
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
        if (status == 404)
            _ERROR("Failed to read file: " + filepath);
    }
    else                // Request was received, but malformed
    {
        _DEBUG("Request not OK");
        status = 400;
    }
    
    // Delete old response and construct new one
    delete response_;
    response_ = new HttpResponse(httpVersion, status, payload);
    _DEBUG("Created HttpResponse with version " + httpVersion + ", status " + to_string(status));

    return sendAll(sockfd, response_->toString());
}
