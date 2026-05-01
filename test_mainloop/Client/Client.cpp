#include "Client.hpp"
#include "../CGI/CGI.hpp"
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
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

Client::Client() : _fd(-1), _epfd(-1), _request() {}

Client::Client(int epfd) : _fd(-1), _epfd(epfd), _request() {}

Client::Client(const Client &obj)
    : _fd(obj._fd), _epfd(obj._epfd), _request(obj._request) {}

const Client &Client::operator=(const Client &obj) {
  if (&obj == this) {
    return *this;
  }
  _fd = obj._fd;
  _request = obj._request;
  _epfd = obj._epfd;
  return *this;
}

void Client::setFd(int fd) { _fd = fd; }

int Client::getFd() const { return _fd; }

void Client::reset() {
  setFd(-1);
  _request.reset();
  _response.reset();
}

int Client::closeConnection() {
  _response.build(_request);
  const char *response = _response.getResponse();
  if (send(_fd, response, strlen(response), 0) ==
      -1) // should we even protect? connection gets closed anyways
    abort();
  return 1;
}

void Client::handleCGI(CGI &cgi) {
  // bool err = false;

  try {
    cgi.initCGI();
    // std::cout << "========= initCGI() succeeded\n";
    cgi.pipeIO();
    // std::cout << "========= pipeIO() succeeded\n";
    cgi.spawnProcess();
    // std::cout << "========= spawnProcess() succeeded\n";
    // cgi.wait(); // TODO do we need wait for CGI to finish executing and
    // writing to the pipe? std::cout << "========= wait() succeeded\n";
    // cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
  } catch (std::exception &e) {
    std::cerr << "exception caught in handleCGI(): " << e.what() << std::endl;
  }
  // if (_response.build(_request) == 1)
  //   err = true;
  // const char *response = _response.getResponse();
  // if (send(_fd, response, strlen(response), 0) ==
  //     -1) // how should we protect here? cut client/close server?
  //   abort();
  // std::vector<char> responseBody = _response.getResponseBody();
  // if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1)
  //   abort();
  // _request.reset();
  // _response.reset();
  // if (err == true)
  //   return 1;
  // std::cout << "SUCCESS\n";
}

void Client::handleCGIOutput(int pipeReadFd) {
  std::stringstream ss;
  char buf[1024];
  ssize_t bytesRead;

  // TODO should check for epoll readiness first
  bytesRead = read(pipeReadFd, buf, 1023);
  buf[bytesRead] = '\0';
  ss << buf;
  std::cout << "\nadded output of CGI to stringstream:\n"
            << ss.str() << std::endl;
}

int Client::loop(std::string input) {
  _bytesRead = 0;
  bool err = false;
  while (_bytesRead < input.length()) {
    if (_request.parseHttpRequest(input, _bytesRead) == 1)
      return closeConnection();
    if (_request.parsingDone() == false)
      return 0;
    _bytesRead += _request.getBytesRead();
    // _request.print();
    if (CGI::isCGIRequest(_request)) {
      CGI cgi(_request, _fd, _epfd);
      handleCGI(cgi);
      return 0;
    }
    if (_response.build(_request) == 1)
      err = true;
    const char *response = _response.getResponse();
    // TODO should check for epoll readiness first
    if (send(_fd, response, strlen(response), 0) ==
        -1) // how should we protect here? cut client/close server?
      abort();
    std::vector<char> responseBody = _response.getResponseBody();
    if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1)
      abort();
    _request.reset();
    _response.reset();
    if (err == true)
      return 1;
    // std::cout << "SUCCESS\n";
  }
  return 0;
}
