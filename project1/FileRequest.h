#ifndef FILE_REQUEST_H_
#define FILE_REQUEST_H_

#include <string>

class HttpRequest;
class HttpResponse;

class FileRequest
{
private:
    HttpRequest *request_;
    HttpResponse *response_;

    // Reads nbytes from sockfd and saves them to a file with the given filename.
    // If nbytes == -1, then reads until the connection is closed.
    bool saveStream(int sockfd, const std::string& filename, int nbytes);
public:
    FileRequest(std::string httpVersion, std::string host, std::string path);
    ~FileRequest();

    HttpResponse *getResponse() const;

    // Sends an HTTP request for the file associated with this FileRequest.
    // Returns true if the request was sent successfully, and false if not.
    bool sendRequest(int sockfd);

    // Receives the HTTP response for the file associated with this FileRequest,
    // and saves the payload with the given filename if the status is 200 OK.
    // Returns true if the response was retrieved/saved successfully and valid, and false if not.
    // Note: the response may be 400/404 and still return true;
    // if response is 200, requires a payload to be valid (optional for others).
    bool recvResponse(int sockfd, const std::string& filename);
};

#endif /* FILE_REQUEST_H_ */
