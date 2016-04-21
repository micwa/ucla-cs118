#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include "HttpMessage.h"
#include "WebUtil.h"

#include <string>

class HttpResponse : public HttpMessage
{
public:
    HttpResponse(std::string version, int status, std::string payload);
};

inline HttpResponse::HttpResponse(std::string version, int status, std::string payload)
    : HttpMessage(ConstructStatusLine(version, status))
{
    setPayload(payload);
}

#endif /* HTTP_RESPONSE_H_ */
