#pragma once
#include "../CGI/CGI.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <ctime>
#include <string>

class Client { 
  public:
    Client();
    // Client(const Client &obj);
    // Client(const t_config &config, const int _sid);
    // Client(int epfd, const t_config &config, const int _sid);
    // Client &operator=(const Client &obj);

    int    loop(std::string &recvBuffer);
    void   init(int epfd, const t_config *config, const int sid, const int clientFd);
    void   reset();

    // getters
    time_t getLastActivity();
    int    getFd() const;
    CGI&    getCGI();

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

    const t_config *_config;
    CGI             _CGI;

    void closeConnection(int reason);
    void   doCGI();
    void   handleCGI();
};
