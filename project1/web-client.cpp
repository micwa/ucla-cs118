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

// Save the payload (if any) from the given HttpResponse to a filename
// extracted from the given path.
static void trySaveResponse(const string& path, HttpResponse *response)
{
    const string DEFAULT_INDEX_HTML = "index.html";
    string contents = response->getPayload();

    if (contents.empty())
        return;

    // Extract filename from path
    int i = path.size() - 1;
    while (i >= 0 && path[i] != '/')
        --i;
    string filename = path.substr(i + 1);
    if (filename.empty())
        filename = DEFAULT_INDEX_HTML;

    // Save file
    ofstream ofs(filename);
    ofs << contents;
    if (ofs.fail())
        _ERROR("Failed to write response: " + filename);
    else
        _DEBUG("Response saved successfully: " + filename);
    ofs.close();
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
        struct addrinfo *allIps = NULL, *ip;
        int sockfd;
        bool isPersistent = false;

        // Establish a connection to the server
        {
            int status;

            if ((status = getIpv4(host, port, &allIps)) != 0)
                errorAndExit(string("getaddrinfo: ") + gai_strerror(status));

            // Try all IPs
            for (ip = allIps; ip != NULL; ip = ip->ai_next)
            {
                if ((sockfd = socket(ip->ai_family, ip->ai_socktype, ip->ai_protocol)) == -1)
                {
                    perror("socket() error");
                    continue;
                }
                if (connect(sockfd, ip->ai_addr, ip->ai_addrlen) == -1)
                {
                    perror("connect() error");
                    continue;
                }
                break;
            }
            if (ip == NULL)
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
                status = request.recvResponse(sockfd);
                if (status)
                {
                    response = request.getResponse();
                    trySaveResponse(path, response);
                }
                else
                    _ERROR("Response was malformed/incomplete");
            }
            else
                _ERROR("Request not sent");

            // Remove file from vector; if vector empty, remove key from hashmap
            paths.pop_back();
            if (paths.empty())
                files.erase(it);

            // Set persistence; if the response was not received properly, counts as not persistent
            string value;
            if (status)
            {
               if ((response->getHeader("Connection", value) && value == "close") ||
                   response->getHttpVersion() != HTTP_VERSION_11)
                   isPersistent = false;
               else
                   isPersistent = true;
            }
        }

        // If HTTP/1.1 (and not "Connection: close") is supported, retrieve more files
        if (isPersistent)
        {
            vector<pair<FileRequest*, string>> requests;

            // Send the files (pipelined)
            for (string path : paths)
            {
                FileRequest *request = new FileRequest(HTTP_VERSION_11, host, path);

                if (request->sendRequest(sockfd))
                    requests.push_back(make_pair(request, path));
                else
                    _ERROR("Pipelined request not sent");
            }
            // Receive and save responses
            for (pair<FileRequest*, string> p : requests)
            {
                FileRequest *request = p.first;
                string path = p.second;

                if (request->recvResponse(sockfd))
                {
                    HttpResponse *response = request->getResponse();
                    trySaveResponse(path, response);
                }
                else
                    _ERROR("Pipelined response was malformed/incomplete");
            }

            // All files (for this connection) have been requested
            files.erase(it);
        }
        
        // Close the connection
        _DEBUG("CONNECTION CLOSED");
        freeaddrinfo(allIps);
        close(sockfd);
    }

    _DEBUG("Client exiting");
    return 0;
}
