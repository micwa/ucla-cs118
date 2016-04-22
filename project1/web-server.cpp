#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    cout << "Invalid number of arguments!\n";
    return 1;
  }

  getaddrinfo(argv[0], argv[1], )
}
