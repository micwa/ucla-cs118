#ifndef FILE_REQUEST_H_
#define FILE_REQUEST_H_

#include <string>

class HttpRequest;
class HttpResponse;

class FileRequest
{
private:
    std::string filename_;
    HttpRequest *request_;
    HttpResponse *response_;
public:
    FileRequest(std::string httpVersion, std::string host, std::string path);
    ~FileRequest();

    HttpResponse *getResponse() const;

    // Sends an HTTP request for the file associated with this FileRequest.
    // Returns true if the request was sent successfully, and false if not.
    bool sendRequest(int sockfd);

    // Receives the HTTP response for the file associated with this FileRequest,
    // and saves the file with a filename extracted from the path.
    // Returns true if the file was retrieved successfully, and false if not.
    bool recvResponse(int sockfd);
};

#endif /* FILE_REQUEST_H_ */
