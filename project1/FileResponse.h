#ifndef FILE_RESPONSE_H_
#define FILE_RESPONSE_H_

#include <string>

class HttpRequest;
class HttpResponse;

class FileResponse
{
private:
    std::string httpVersion_;   // HTTP version from request
    HttpRequest *request_;
    HttpResponse *response_;

    // Helper function for creating and sending an HttpResponse.
    bool sendHttpResponse(int sockfd, const std::string& httpVersion, int status, int payloadLength = -1);
public:
    FileResponse();
    ~FileResponse();

    HttpRequest *getRequest() const;

    // Receive an HTTP request through sockfd.
    // Returns 0 if timeout/EOF encountered before receiving the whole request,
    // -1 on error (recv() error or malformed request), and 1 otherwise.
    int recvRequest(int sockfd);

    // Construct and send a response based on the request that was received.
    // If no request was received (i.e., recvRequest() had not been called or
    // returned false), returns false. If any bytes had been received, this
    // will either send a 200/404, or a 400 response.
    // Returns true if the response was sent successfully, and false if not.
    bool sendResponse(int sockfd, const std::string& baseDir);
};

#endif /* FILE_RESPONSE_H_ */
