#include <string>
#include <unordered_map>

class HttpMessage
{
private:
    std::string firstLine_;
    std::string httpVersion_;
    std::unordered_map<std::string, std::string> headers_;
    std::string payload_;
public:
    HttpMessage(std::string firstLine);

    std::string getHttpVersion() const;
    std::string getFirstLine() const;
    void setFirstLine(const std::string firstLine);

    std::string getHeader(const std::string header) const;
    void setHeader(const std::string header, const std::string value);
    std::string getPayload() const;
    void setPayload(const std::string payload);

    // Returns a string representation of this HttpMessage
    std::string toString();
};