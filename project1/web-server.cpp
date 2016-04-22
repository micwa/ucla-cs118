#include "web-server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

using namespace std;

int main(int argc, char** argv) 
{
  if (argc != 3)
  {
    cout << "Incorrect number of arguments!\n";
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
}

