#include "FileResponse.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

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
    if (res == 0)
        return 0;
    
    // If there's no HTTP version, just use the default.
    // An HttpRequest will only be created if the line is valid anyway.
    if (!checkRequestLineValid(firstLine))
        res = -1;
    httpVersion_ = getVersionFromLine(firstLine);   
    if (httpVersion_ == "")
        httpVersion_ = HTTP_DEFAULT_VERSION;

    // Receive subsequent lines
    vector<string> lines;
    int res2 = readlinesUntilEmpty(sockfd, lines);
    if (res == -1)
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

bool FileResponse::sendHttpResponse(int sockfd, const string& httpVersion, int status, int payloadLength)
{
    delete response_;
    response_ = new HttpResponse(httpVersion, status);
    _DEBUG("Created HttpResponse with version " + httpVersion + ", status " + to_string(status));
    if (payloadLength >= 0)
        response_->setHeader("Content-length", to_string(payloadLength));

    return sendAll(sockfd, response_->headerToString());
}

bool FileResponse::sendResponse(int sockfd, const string& baseDir)
{
    if (httpVersion_ == "")
        return false;

    if (!request_)      // Request was received, but malformed => send 400
    {
        _DEBUG("Request not OK");
        return sendHttpResponse(sockfd, httpVersion_, 400);
    }

    // Parse request for payload path (file path)
    string line = request_->getFirstLine();
    string filepath = baseDir + getPathFromRequestLine(line);

    // Read file, and 404 if not found (or I/O error)
    _DEBUG("Request OK, reading from: " + filepath);
    ifstream ifs(filepath);
    int bytesLeft;

    // Get the payload length (if error, 404)
    struct stat s;
    int res = stat(filepath.c_str(), &s);
    if (res == 0)
        bytesLeft = s.st_size;
    else
        bytesLeft = -1;

    if (ifs && bytesLeft >= 0)
    {
        // Sent HttpResponse, then the file
        if (!sendHttpResponse(sockfd, httpVersion_, 200, bytesLeft))
            return false;

        try {
            char buf[MAX_SENT_BYTES];

            // Send the file, in increments of MAX_SENT_BYTES
            while (bytesLeft > 0)
            {
                int toRead = min(MAX_SENT_BYTES, bytesLeft);
                ifs.read(buf, toRead);
                if (ifs.fail())
                    return false;
                if (!sendAll(sockfd, string(buf, toRead)))
                    return false;
                bytesLeft -= toRead;
            }
            ifs.close();
        } catch (...) {
            // Can't send anything else to remedy the situation, so just return
            return false;
        }
        return true;
    }
    else
    {
        _ERROR("Failed to read file: " + filepath);
        return sendHttpResponse(sockfd, httpVersion_, 404);
    }
}
