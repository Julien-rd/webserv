#pragma once
#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <ctime>
#include <sstream>
#include <string>

class Client {  // Fix: This has too many responsibilities: extract everything from cgi to the
                // existing cgi classes or a cgi handler even the resetting.
                // also the request/response workflow can be outsources to HTTP handling
  public:
    Client();
    // Client(const t_config &config, const int _sid);
    // Client(int epfd, const t_config &config, const int _sid);
    Client(const Client &obj);
    // Client &operator=(const Client &obj);

    int    loop(std::string &recvBuffer);
    void   doCGI();
    size_t getBytesRead();
    void   readCGIPipe(int pipeReadFd);
    void   init(int epfd, const t_config *config, const int sid, const int clientFd);
    void   reset();

    // getters
    time_t getLastActivity();
    int    getFd() const;

    // setters
    void setLastActivity();

  private:
    int          _fd;
    int          _sid;
    int          _epfd;
    HttpRequest  _request;
    HttpResponse _response;
    size_t       _bytesRead;
    time_t       _lastActivity;

    // std::stringstream _CGIResponseStream;
    std::string     _CGIResponseStr;
    ssize_t         _CGIResponseLen;
    pid_t           _CGIPid;
    CGIResponse     _CGIResponse;
    const t_config *_config;
    CGI             _CGI;

    void closeConnection();
};
