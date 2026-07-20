#pragma once
#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "../Error/Error.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include <ctime>
#include <sstream>
#include <string>

class Client : public Error {
private:
  int          _fd;
  int          _sid;
  int          _epfd;
  HttpRequest  _request;
  HttpResponse _response;
  size_t       _bytesRead;
  time_t       _lastActivity;

  std::stringstream _CGIResponseStream;
  ssize_t           _CGIResponseLen;
  pid_t             _CGIPid;
  CGIResponse       _CGIResponse;
  const t_config&   _config;
  CGI               _CGI;

  void closeConnection();

public:
  Client(const t_config& config, const int _sid);
  Client(int epfd, const t_config& config, const int _sid);
  Client(const Client& obj);
  const Client& operator=(const Client& obj);

  int    loop(std::string recvBuffer);
  void   doCGI();
  size_t getBytesRead();
  void   readCGIPipe(int pipeReadFd);
  void   reset();
  void   setFd(int fd);
  void  setLastActivity();
  time_t  getLastActivity();
  int    getFd() const;
};

