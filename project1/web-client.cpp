#include "constants.h"
#include "FileRequest.h"
#include "HttpResponse.h"
#include "WebUtil.h"
#include "logerr.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <utility>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

namespace std
{
    template <>
    struct hash<pair<string, int>>
    {
        size_t operator()(const pair<string, int>& x) const
        {
            return hash<string>()(x.first) ^ hash<int>()(x.second);
        }
    };
}

static void errorAndExit(const string& msg)
{
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

// Extracts the filename from the given path.
static string filenameFromPath(const string& path)
{
    const string DEFAULT_INDEX_HTML = "index.html";

    // Extract filename from path
    int i = path.size() - 1;
    while (i >= 0 && path[i] != '/')
        --i;
    string filename = path.substr(i + 1);
    if (filename.empty())
        filename = DEFAULT_INDEX_HTML;

    return filename;
}

int main(int argc, char *argv[])
{
    // Parse arguments into (host, port) => [file]
    unordered_map<pair<string, int>, vector<string>> files;

    for (int i = 1; i < argc; ++i)
    {
        string url = argv[i];
        string host, path;
        int port;

        if (parseUrl(url, host, port, path))
        {
            files[make_pair(host, port)].push_back(path);
            _DEBUG("Address = " + host + ":" + to_string(port) + path);
        }
    }

    // For each (host, port) pair:
    //  - try to establish a persistent connection (HTTP/1.1) and send all requests at once
    //  - if only HTTP/1.0 supported, send requests one at a time
    //  (while gradually removing keys one by one from the map)
    for (auto it = files.begin(); it != files.end(); it = files.begin())
    {
        string host           = it->first.first;
        int port              = it->first.second;
        vector<string>& paths = it->second;
        int sockfd;
        bool isPersistent = false;

        // Establish a connection to the server
        {
            struct addrinfo *servAddr = NULL, *addr;
            int status;

            if ((status = getIpv4(host, port, &servAddr)) != 0)
                errorAndExit(string("getaddrinfo: ") + gai_strerror(status));

            // Try all IPs
            for (addr = servAddr; addr != NULL; addr = addr->ai_next)
            {
                if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
                {
                    perror("socket() error");
                    continue;
                }
                if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) == -1)
                {
                    perror("connect() error");
                    continue;
                }
                break;
            }

            freeaddrinfo(servAddr);
            if (addr == NULL)
                errorAndExit("No connection possible for host: " + host);
        }

        // Retrieve first file
        {
            string path = paths.back();
            FileRequest request(HTTP_VERSION_11, host, path);
            HttpResponse *response;

            int status = request.sendRequest(sockfd);
            if (status)     // Receive and save file
            {
                string filename = filenameFromPath(path);
                status = request.recvResponse(sockfd, filename);
                if (status) // Response was successfully received; could be 200, 400, or 404
                    response = request.getResponse();
                else
                    _ERROR("Response was malformed/incomplete");
            }
            else
                _ERROR("Request not sent");

            // Remove file from vector; if vector empty, remove key from hashmap
            paths.pop_back();
            if (paths.empty())
                files.erase(it);
            else
            {
                // If HTTP/1.1 (and not "Connection: close") is supported and there are files
                // left to retrieve, set isPersistent = true.
                // Note: if the response was not received properly, counts as not persistent.
                string value;
                if (status)
                {
                   if (response->getHttpVersion() != HTTP_VERSION_11 ||
                       (response->getHeader("Connection", value) && value == "close"))
                       isPersistent = false;
                   else
                       isPersistent = true;
                }
            }
        }

        // Retrieve more files (pipelined)
        if (isPersistent)
        {
            vector<pair<FileRequest*, string>> requests;

            // Send the files
            for (string path : paths)
            {
                FileRequest *request = new FileRequest(HTTP_VERSION_11, host, path);

                if (request->sendRequest(sockfd))
                    requests.push_back(make_pair(request, path));
                else
                {
                    _ERROR("Pipelined request not sent");
                    delete request;
                }
            }
            // Receive and save responses
            for (pair<FileRequest*, string> p : requests)
            {
                FileRequest *request = p.first;
                string path = p.second;
                string filename = filenameFromPath(path);

                if (!request->recvResponse(sockfd, filename))
                    _ERROR("Pipelined response was malformed/incomplete");
                delete request;
            }

            // All files (for this connection) have been requested
            files.erase(it);
        }
        
        // Close the connection
        _DEBUG("CONNECTION CLOSED");
        close(sockfd);
    }

    _DEBUG("Client exiting");
    return 0;
}
