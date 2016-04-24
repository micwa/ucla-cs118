#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"

#include <iostream>
#include <vector>

using namespace std;

int main() 
{
    HttpRequest req("1.1", "www.google.com", "/mail");
    HttpResponse resp("1.1", 200, "some_mail_here");
    cout << req.toString();
    cout << resp.toString() << endl;
    cout << "Versions: " << req.getHttpVersion() << " " << resp.getHttpVersion() << endl;

    // Parsing first lines of HTTP messages
    cout << getVersionFromLine("HTTP/1.1 200 OK") << endl;
    cout << getStatusCodeFromStatusLine("HTTP/1.1 404 not found") << endl;
    cout << getPathFromRequestLine("GET /some_path HTTP/1.1") << endl;

    // makeHttpRequest()
    vector<string> lines = { "Host: abc.com\r\n", "Another-header : some value \r\n"};
    HttpRequest *req2 = makeHttpRequest("1.1", "abc.com", "/foo", lines);
    cout << req2->toString();

    // parseUrl()
    string host, path;
    int port;
    string url1 = "bob.com";
    string url2 = "http://localhost:8080/foo/bar.html";

    cout << parseUrl(url1, host, port, path);
    cout << ":" << host << ":" << port << ":" << path << endl;
    cout << parseUrl(url2, host, port, path);
    cout << ":" << host << ":" << port << ":" << path << endl;

    cout << "--- END" << endl;
    return 0;
}
