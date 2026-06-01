#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "../Error/Error.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include <sstream>
#include <string>

class Client : public Error {
private:
  int          _fd;
  int          _epfd;
  HttpRequest  _request;
  HttpResponse _response;
  size_t       _bytesRead;

  std::stringstream _CGIResponseStream;
  ssize_t           _CGIResponseLen;
  pid_t             _CGIPid;
  CGIResponse       _CGIResponse;

  int closeConnection();

public:
  Client();
  Client(int epfd);
  Client(const Client& obj);
  const Client& operator=(const Client& obj);

  int    loop(std::string input);
  bool   doCGI(void);
  size_t getBytesRead();
  void   readCGIPipe(int pipeReadFd);
  void   reset();
  void   setFd(int fd);
  int    getFd() const;
};

#endif
