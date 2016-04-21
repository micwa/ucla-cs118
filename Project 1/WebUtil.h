#ifndef WEB_UTIL_H_
#define WEB_UTIL_H_

#include <string>

inline
std::string HttpStatusDescription(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 400:
        return "Bad request";
    case 404:
        return "Not found";
    }
}

inline
std::string ConstructGetRequest(std::string version, std::string path)
{
    return "GET " + path + "HTTP/" + version;
}

inline
std::string ConstructStatusLine(std::string version, int status)
{
    return "HTTP/" + version + " " + std::to_string(status) +
           " " + HttpStatusDescription(status);
}

#endif /* WEB_UTIL_H_ */
