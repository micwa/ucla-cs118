#include <iostream>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"

using namespace std;

int main() 
{
    HttpRequest req("1.1", "www.google.com", "/mail");
    HttpResponse resp("1.1", 200, "some_mail_here");
    cout << req.toString();
    cout << resp.toString() << endl;
    cout << "Versions: " << req.getHttpVersion() << " " << resp.getHttpVersion() << endl;

    cout << getVersionFromLine("HTTP/1.1 200 OK") << endl;
    cout << getStatusCodeFromStatusLine("HTTP/1.1 404 not found") << endl;
    cout << getPathFromRequestLine("GET /some_path HTTP/1.1") << endl;
    return 0;
}
