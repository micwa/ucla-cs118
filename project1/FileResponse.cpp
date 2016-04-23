#include "FileResponse.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

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
    // Receive the first line
    string firstLine;
    int res = readline(sockfd, firstLine);
    if (res == 0 || res == -1)
        return false;

    // Receive subsequent lines
    vector<string> lines;
    res = readlinesUntilEmpty(sockfd, lines);
    if (res == 0 || res == -1)
        return false;

    // Create an HttpRequest
    string version = getVersionFromLine(firstLine);
    string path = getPathFromRequestLine(firstLine);
    string host;

    for (string s : lines)      // Look for a "Host:" header
    {
        if (tolower(s.substr()
    }
    delete request_;
    request = new HttpRequest(

    // Receive a possible payload

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
        string filepath = GetPathFromRequestLine(line);

        // Read file, and 404 if not found (or I/O error)
        _DEBUG("Request OK, reading from: " + baseDir + filepath);
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
        _DEBUG("Request not OK");
        status = 400;
    }
    
    // Delete old response and construct new one
    delete response_;
    response_ = new HttpResponse(httpVersion, status, payload);
    _DEBUG("Created HttpResponse with version " + httpVersion + ", status " + to_string(status));

    return sendAll(sockfd, response_->toString());
}
