#ifndef HTTP_MESSAGE_H_
#define HTTP_MESSAGE_H_

#include <string>
#include <unordered_map>

class HttpMessage
{
private:
    std::string firstLine_;
    std::string httpVersion_;
    std::unordered_map<std::string, std::string> headers_;
    std::string payload_;

    void setFirstLine(const std::string firstLine);
public:
    HttpMessage(std::string firstLine);

    std::string getHttpVersion() const;
    std::string getFirstLine() const;

    bool getHeader(const std::string header, std::string& value) const;
    void setHeader(const std::string header, const std::string value);
    std::string getPayload() const;
    void setPayload(const std::string payload);

    // Returns a string representation of this HttpMessage
    std::string toString();
};

#endif /* HTTP_MESSAGE_H_ */
