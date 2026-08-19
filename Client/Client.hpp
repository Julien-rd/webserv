#pragma once
#include "../CGI/CGI.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <ctime>
#include <string>

// enum clientStatus { CLIENT_KEEP, CLIENT_CLOSE};

class Client {
  public:
    Client();
    // Client(const Client &obj);
    // Client(const t_config &config, const int _sid);
    // Client(int epfd, const t_config &config, const int _sid);
    // Client &operator=(const Client &obj);

    // int  loop(std::string &recvBuffer);

    bool parseRecvBuffer(std::string &recvBuffer);
    bool parsePending();
    bool sendResponse();

    void init(int epfd, const t_config *config, const int sid, const int clientFd);
    void reset();
    bool recvBufferIsParsed() const;
    int        prepareSendCGI(int pipeReadFd);
    bool keepConnection() const;

    // getters
    time_t getLastActivity();
    int    getFd() const;
    CGI   &getCGI();

    // setters
    void setLastActivity();

  private:
    int               _fd;
    int               _sid;
    int               _epfd;
    HttpRequest       _request;
    HttpResponse      _response;
    std::vector<char> _fullResponse;
    size_t            _bytesRead;
    time_t            _lastActivity;
    size_t            _bytesSent;
    size_t            _responseSize;
    std::string       _recvBuffer;
    size_t            _maxRecvBuffer;

    const t_config *_config;
    CGI             _CGI;

    bool closeConnection(int reason);
    bool updateEpoll(const unsigned int &event);
    void doCGI();
    void handleCGI();
};
