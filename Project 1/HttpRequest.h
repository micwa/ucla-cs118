#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include "HttpMessage.h"
#include "WebUtil.h"

#include <string>

class HttpRequest : public HttpMessage
{
public:
    HttpRequest(std::string version, std::string host, std::string path);
};

inline
HttpRequest::HttpRequest(std::string version, std::string host, std::string path)
    : HttpMessage(ConstructGetRequest(version, path))
{
    setHeader("Host", host);
}

#endif /* HTTP_REQUEST_H_ */
