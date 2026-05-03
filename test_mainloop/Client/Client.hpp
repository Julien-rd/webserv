#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../CGI/CGI.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include <sstream>
#include <string>

class Client {
private:
  int               _fd;
  int               _epfd;
  HttpRequest       _request;
  HttpResponse      _response;
  size_t            _bytesRead;
  std::stringstream _CGIResponseStream;

  int closeConnection();

public:
  Client();
  Client(int epfd);
  Client(const Client& obj);
  const Client& operator=(const Client& obj);

  int    loop(std::string input);
  size_t getBytesRead();
  void   handleCGI(CGI& cgi);
  void   handleCGIOutput(int pipeReadFd);
  void   reset();
  void   setFd(int fd);
  int    getFd() const;
};

#endif
