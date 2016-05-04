#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include "HttpMessage.h"
#include "WebUtil.h"

#include <string>

class HttpResponse : public HttpMessage
{
private:
    int status_;
public:
    HttpResponse(std::string version, int status);
    int getStatus() const;
};

inline HttpResponse::HttpResponse(std::string version, int status)
    : HttpMessage(constructStatusLine(version, status)),
      status_(status)
{}

inline int HttpResponse::getStatus() const { return status_; }

#endif /* HTTP_RESPONSE_H_ */
