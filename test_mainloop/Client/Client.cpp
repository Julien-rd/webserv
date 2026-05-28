#include "Client.hpp"
#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

Client::Client()
    : _fd(-1), _epfd(-1), _request(), _CGIResponseLen(0), _CGIPid(-1),
      _CGIResponse(_CGIResponseStream, _CGIResponseLen) {}

Client::Client(int epfd)
    : _fd(-1), _epfd(epfd), _request(), _CGIResponseLen(0), _CGIPid(-1),
      _CGIResponse(_CGIResponseStream, _CGIResponseLen) {}

Client::Client(const Client& obj)
    : _fd(obj._fd), _epfd(obj._epfd), _request(obj._request),
      _CGIResponseLen(obj._CGIResponseLen), _CGIPid(obj._CGIPid),
      _CGIResponse(obj._CGIResponse) {}

const Client& Client::operator=(const Client& obj) {
  if (&obj == this) {
    return *this;
  }
  _fd = obj._fd;
  _epfd = obj._epfd;
  _request = obj._request;
  _CGIResponseLen = obj._CGIResponseLen;
  _CGIPid = obj._CGIPid;
  return *this;
}

void Client::setFd(int fd) { _fd = fd; }

int Client::getFd() const { return _fd; }

void Client::reset() {
  setFd(-1);
  _request.reset();
  _response.reset();
  _CGIResponse.reset();
}

int Client::closeConnection() {
  _response.build(_request);
  const char* response = _response.getResponse();
  if (send(_fd, response, strlen(response), 0) ==
      -1) // should we even protect? connection gets closed anyways
    abort();
  return 1;
}

void Client::startCGI(CGI& cgi) {
  // bool err = false;

  try {
    cgi.initCGI();
    // std::cout << "========= initCGI() succeeded\n";
    cgi.pipeIO();
    // std::cout << "========= pipeIO() succeeded\n";
    cgi.spawnProcess();
    _CGIPid = cgi.getPid();
    // std::cout << "========= spawnProcess() succeeded\n";
    // writing to the pipe? std::cout << "========= wait() succeeded\n";
  } catch (std::exception& e) {
    std::cerr << "exception caught in startCGI(): " << e.what() << std::endl;
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

void Client::handleCGIResponse(int pipeReadFd) {
  char    buf[BUFFER_SIZE];
  ssize_t bytesRead;

  bytesRead = read(pipeReadFd, buf, BUFFER_SIZE - 1);
  std::cout << "bytes read from CGI pipe: " << bytesRead << std::endl;
  if (bytesRead == -1) {
    _CGIResponseLen = 0;
    _CGIResponseStream.clear();
    throw std::runtime_error("read() failed in Client::handleCGIResponse");
  }
  if (bytesRead == 0) {
    int res = waitpid(_CGIPid, NULL, WNOHANG);
    if (res == -1) {
      throw std::runtime_error("waitpid() failed in handleCGIResponse()");
    }
    if (res == 0) { // TODO this branch is untested
      kill(_CGIPid, SIGKILL);
      waitpid(_CGIPid, NULL, 0);
    }
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, pipeReadFd, NULL) == -1) {
      throw std::runtime_error("couldn't remove CGI pipe from epoll");
    }
    close(pipeReadFd);
    std::cout << "\nbuilding HttpResponse from CGI Response:\n{\n"
              << _CGIResponseStream.str() << "\n}\n";
    if (_CGIResponse.build(_request) == 1) {
      throw std::runtime_error("couldn't build HttpResponse from CGI Response");
      // TODO handle error
    }
    const char* response = _CGIResponse.getResponse();
    // std::cout << "\nHttpResponse Response:\n" << response << std::endl;
    if (send(_fd, response, strlen(response), 0) ==
        -1) // how should we protect here? cut client/close server?
      abort();
    std::vector<char> responseBody = _CGIResponse.getResponseBody();
    if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1)
      abort();
    _request.reset();
    _CGIResponse.reset();
  } else {
    buf[bytesRead] = '\0';
    _CGIResponseLen += bytesRead;
    _CGIResponseStream.write(buf, bytesRead + 1);
    // std::cout << "added " << " to stringstream" << std::endl;
  }
}

bool Client::doCGI(void) {
  if (!CGI::isCGIRequest(_request)) {
    return 1;
  }
  CGI cgi(_request, _fd, _epfd);
  startCGI(cgi);
  return 0;
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
    _request.parseURI();
    if (doCGI() == false) {
      return 0;
    }
    if (_response.build(_request) == 1)
      err = true;
    const char* response = _response.getResponse();
    // std::cout << "response:\n" << response << std::endl;
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
