#include <iostream>
#include "HttpRequest.h"
#include "HttpResponse.h"

using namespace std;

int main() 
{
    HttpRequest req("1.1", "www.google.com", "/mail");
    HttpResponse resp("1.1", 200, "some_mail_here");
    cout << req.toString();
    cout << resp.toString();
    return 0;
}
