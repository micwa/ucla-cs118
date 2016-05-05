#include "FileRequest.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

#include <climits>
#include <fstream>
#include <string>
#include <vector>

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
    return sendAll(sockfd, request_->headerToString());
}

bool FileRequest::saveStream(int sockfd, const string& filename, int nbytes)
{
    int bytesLeft = nbytes;
    int total = 0;
    ofstream ofs(filename);

    if (bytesLeft == -1)
        bytesLeft = INT_MAX;
    if (ofs)
    {
        try {
            string result;
            while (bytesLeft > 0)
            {
                int toRecv = min(bytesLeft, MAX_RECV_BYTES);
                bool res = recvAll(sockfd, result, toRecv);

                // If nbytes == -1, connection close/timeout/error means EOF
                if (!res)
                    return nbytes == -1;

                ofs << result;
                total += toRecv;
                if (nbytes != -1)
                    bytesLeft -= toRecv;
            }
            ofs.close();
            _DEBUG("Payload saved successfully: " + filename + " (" + to_string(total) + " bytes)");
        } catch (...) {
            _ERROR("Can not write to file: " + filename);
            return false;
        }
        return true;
    }
    else
    {
        _ERROR("Can not create file to save payload: " + filename);
        return false;
    }
}

// To be well-formed, the response must have:
//  - HTTP version and status
//  - well-formed header lines
bool FileRequest::recvResponse(int sockfd, const string& filename)
{
    // Receive the first line
    string firstLine;
    int res = readline(sockfd, firstLine);
    if (res == 0)
	return false;

    string httpVersion = getVersionFromLine(firstLine);   
    int status = getStatusCodeFromStatusLine(firstLine);
    if (httpVersion == "" || status == -1)
        res = -1;

    // Receive subsequent lines
    vector<string> lines;
    int res2 = readlinesUntilEmpty(sockfd, lines);
    if (res == -1 || (res2 == 0 && status == 200) || res2 == -1)
        return false;

    // Create HttpResponse from lines read
    delete response_;
    response_ = makeHttpResponse(httpVersion, status, lines);
    if (!response_)
        return false;

    // Receive payload only if status is 200
    if (status == 200)
    {
        int length = -1;
        string length_str;
        response_->getHeader("Content-length", length_str);
        try {
            length = stoi(length_str);
        } catch (...) {
            length = -1;
        }
        if (!saveStream(sockfd, filename, length))
            return false;
    }
    else
    {
        _DEBUG("Response not OK, status " + to_string(status));
    }

    _DEBUG("HttpResponse received successfully");
    return true;
}
