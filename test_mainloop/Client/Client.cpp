#include "Client.hpp"
#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <exception>
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

Client::Client(const t_config& config, const int sid)
    : _fd(-1), _sid(sid), _epfd(-1), _request(), _response(config, sid),
      _CGIResponseLen(0), _CGIPid(-1),
      _CGIResponse(_CGIResponseStream, _CGIResponseLen, config, sid),
      _config(config), _CGI(_request, _fd, _epfd, _config.servers.at(_sid),
                            _config.servers.at(_sid).cgiConfigs) {}

Client::Client(int epfd, const t_config& config, const int sid)
    : _fd(-1), _sid(sid), _epfd(epfd), _request(), _response(config, sid),
      _CGIResponseLen(0), _CGIPid(-1),
      _CGIResponse(_CGIResponseStream, _CGIResponseLen, config, sid),
      _config(config), _CGI(_request, _fd, _epfd, _config.servers.at(_sid),
                            _config.servers.at(_sid).cgiConfigs) {}

Client::Client(const Client& obj)
    : _fd(obj._fd), _sid(obj._sid), _epfd(obj._epfd), _request(obj._request),
      _response(obj._response), _CGIResponseLen(obj._CGIResponseLen),
      _CGIPid(obj._CGIPid), _CGIResponse(obj._CGIResponse),
      _config(obj._config), _CGI(obj._CGI) {}

const Client& Client::operator=(const Client& obj) {
  if (&obj == this) {
    return *this;
  }
  _fd = obj._fd;
  _sid = obj._sid;
  _epfd = obj._epfd;
  _request = obj._request;
  _CGIResponseLen = obj._CGIResponseLen;
  _CGIPid = obj._CGIPid;
  return *this;
}

void Client::setFd(int fd) {
  _fd = fd;
  _CGI.setClientFd(fd);
}

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

void Client::readCGIPipe(int pipeReadFd) {
  char    buf[BUFFER_SIZE];
  ssize_t bytesRead;

  bytesRead = read(pipeReadFd, buf, BUFFER_SIZE - 1);
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
    // std::cout << "\nbuilding HttpResponse from CGI Response:\n{\n"
    //           << _CGIResponseStream.str() << "\n}\n";
    _CGIResponse.setCGIResponseStr(_CGIResponseStream.str());
    _CGIResponse.setCGIResponseLen(_CGIResponseLen);
    if (_CGIResponse.build(_request) == 1) {
      closeConnection();
      return;
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
    // std::cout << "adding (( " << buf << " )) to _CGIResponseStream\n";
    _CGIResponseLen += bytesRead;
    _CGIResponseStream.write(buf, bytesRead + 1);
    // std::cout << "_CGIResponseStream becamse: ((" << _CGIResponseStream.str()
    //           << " ))" << std::endl;
  }
}

bool Client::doCGI(void) {
  try {
    // std::cout << " in doCGI() => _config.servers.at(_sid).ip: "
    //           << _config.servers.at(_sid).cgiConfigs.size() << "\n";
    // std::cout << " in doCGI() => _config.servers.at(_sid).port: "
    //           << _config.servers.at(_sid).port << "\n";
    if (!_CGI.scriptFileExists()) {
      return closeConnection();
    }
    _CGI.initCGI();
    // std::cout << "========= initCGI() succeeded\n";
    _CGI.pipeIO();
    // std::cout << "========= pipeIO() succeeded\n";
    _CGI.spawnProcess();
    _CGIPid = _CGI.getPid();
    // std::cout << "========= spawnProcess() succeeded\n";
    // writing to the pipe? std::cout << "========= wait() succeeded\n";
  } catch (std::exception& e) {
    std::cerr << "exception caught in doCGI(): " << e.what() << std::endl;
    return closeConnection();
  }
  return 0;
}

int Client::loop(std::string input) {
  _bytesRead = 0;
  bool err = false;
  while (_bytesRead < input.length()) {
    if (_request.parseHttpRequest(input, _bytesRead) == 1)
      return closeConnection();
    if (_request.parsingDone() == false) {
      return 0;
    }
    _bytesRead += _request.getBytesRead();
    // _request.print();
    if (_request.parseURIContent() == 1) {
      return closeConnection();
    }
    if (_CGI.isCGIRequest(_request)) {
      std::cout << "==> found a CGI request\n";
      doCGI();
      _request.reset();
      _response.reset();
      continue;
    }
    if (_response.build(_request) == 1)
      err = true;
    const char* response = _response.getResponse();
    // std::cout << "response:\n" << response << std::endl;
    if (send(_fd, response, strlen(response), 0) ==
        -1) // TODO how should we protect here? cut client/close server?
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
