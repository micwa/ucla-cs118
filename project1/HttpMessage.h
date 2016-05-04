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

    void setFirstLine(const std::string firstLine);
public:
    HttpMessage(std::string firstLine);

    std::string getHttpVersion() const;
    std::string getFirstLine() const;

    bool getHeader(const std::string header, std::string& value) const;
    void setHeader(const std::string header, const std::string value);

    // Returns a string representation of this HttpMessage's header,
    // that is, the first line + headers + empty line.
    std::string headerToString();
};

#endif /* HTTP_MESSAGE_H_ */
