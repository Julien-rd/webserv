#include "client.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Client::Client() { ; }

#include <sys/socket.h>
#include <sys/types.h>

void Client::setFd(int fd){
  _fd = fd;
}

int Client::getFd() const{
  return _fd;
}

void Client::reset() {
  setFd(-1);
  _request.reset();
  _response.reset();
}

int Client::closeConnection() {
  _response.build(_request);
  const char *response = _response.getResponse();
  if (send(_fd, response, strlen(response), 0) == -1) // should we even protect? connection gets closed anyways
    abort();
  return 1;
}

int Client::loop(std::string input) {
  _bytesRead = 0;
  while (_bytesRead < input.length()) {
    if (_request.parseHttpRequest(input, _bytesRead) == 1)
      return closeConnection();
    if (_request.parsingDone() == false)
      return 0;
    _bytesRead += _request.getBytesRead();
    if (_request.parsingDone() == true) {
      _response.build(_request);
      const char *response = _response.getResponse();
      if (send(_fd, response, strlen(response), 0) == -1) // how should we protect here? cut client/close server?
        abort();
      _request.reset();
      _response.reset();
      std::cout << "SUCCESS\n";
    } else
      return 0;
  }
  return 0;
}

