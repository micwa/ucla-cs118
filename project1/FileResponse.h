#ifndef FILE_RESPONSE_H_
#define FILE_RESPONSE_H_

#include "HttpRequest.h"
#include "HttpResponse.h"

#include <string>

class FileResponse
{
private:
    std::string httpVersion_;
    HttpRequest *request_;
    HttpResponse *response_;
public:
    FileResponse();
    ~FileResponse();

    // Receive an HTTP request through sockfd.
    // Returns true if any bytes are received, and false if not.
    bool recvRequest(int sockfd);

    // Construct and send a response based on the request that was received.
    // If no request was received (i.e., recvRequest() had not been called or
    // returned false), returns false. If any bytes had been received, this
    // will either send a 200/404, or a 400 response.
    // Returns true if the response was sent successfully, and false if not.
    bool sendResponse(int sockfd, const std::string& baseDir);
};

#endif /* FILE_RESPONSE_H_ */
