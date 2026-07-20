#include "Client.hpp"
#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>

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
                            _config.servers.at(_sid).cgiConfigs){}

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

void  Client::setLastActivity() {
    time(&_lastActivity);
}

time_t  Client::getLastActivity() {
    return _lastActivity;
}

int Client::getFd() const { return _fd; }

void Client::reset() {
  setFd(-1);
  _request.reset();
  _response.reset();
  _CGIResponse.reset();
}

void Client::closeConnection() {
  _response.build(_request);
  const char* response = _response.getResponse();
  if (send(_fd, response, strlen(response), 0) ==
      -1) 
          std::cerr << "read() failed in Client::handleCGIResponse(): " << strerror(errno) << "\n";
          // NOTFINISHED: i have no idea whats open here and what this function is responsible for
    return ;
}

void Client::readCGIPipe(int pipeReadFd) {
  char    buf[BUFFER_SIZE];
  ssize_t bytesRead;

  bytesRead = read(pipeReadFd, buf, BUFFER_SIZE - 1);
  if (bytesRead == -1) {
    _CGIResponseLen = 0;
    _CGIResponseStream.clear();
    std::cerr << "read() failed in Client::handleCGIResponse(): " << strerror(errno) << "\n";
    return; // NOTFINISHED: i have no idea whats open here and what this function is responsible for
  }
  if (bytesRead == 0) {
    int res = waitpid(_CGIPid, NULL, WNOHANG);
    if (res == -1) {
      std::cerr << "waitpid() failed in handleCGIResponse(): " << strerror(errno) << "\n";
      return; // NOTFINISHED: i have no idea whats open here and what this function is responsible for
    }
    if (res == 0) { // TODO this branch is untested
      kill(_CGIPid, SIGKILL);
      waitpid(_CGIPid, NULL, 0);
    }
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, pipeReadFd, NULL) == -1) {
        std::cerr << "epoll_ctl() DEL failed in handleCGIResponse(): " << strerror(errno) << "\n";
        return; // NOTFINISHED: i have no idea whats open here and what this function is responsible for
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
    // std::cout << "\nHttpResponse Response:\n" << response << "]" <<std::endl;
    if (send(_fd, response, strlen(response), 0) == -1) {//Fix: we are not allowed to use strlen right
        std::cerr << "send() failed in handleCGIResponse(): " << strerror(errno) << "\n";
        return; // NOTFINISHED: i have no idea whats open here and what this function is responsible for
    }
    std::vector<char> responseBody = _CGIResponse.getResponseBody();
    for (unsigned int i = 0; i < responseBody.size(); ++i) {
        std::cout << responseBody.at(i);
    }
    std::cout << std::endl;
    if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1) {
        std::cerr << "epoll_ctl() DEL failed in handleCGIResponse(): " << strerror(errno) << "\n";
        return; // NOTFINISHED: i have no idea whats open here and what this function is responsible for
    }
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

void Client::doCGI(void) {
    // std::cout << " in doCGI() => _config.servers.at(_sid).ip: "
    //           << _config.servers.at(_sid).cgiConfigs.size() << "\n";
    // std::cout << " in doCGI() => _config.servers.at(_sid).port: "
    //           << _config.servers.at(_sid).port << "\n";
    if (!_CGI.scriptFileExists()) {
        closeConnection();
        return;
    }
    if (!_CGI.initCGI()) {
        closeConnection();
        return;
    }
    // std::cout << "========= initCGI() succeeded\n";
    if (!_CGI.pipeIO()) {
        closeConnection();
        return;
    }
    // std::cout << "========= pipeIO() succeeded\n";
    if (!_CGI.spawnProcess() ) {
        closeConnection();
        return;
    }
    _CGIPid = _CGI.getPid();
    // std::cout << "========= spawnProcess() succeeded\n";
    // writing to the pipe? std::cout << "========= wait() succeeded\n";
  }

int Client::loop(std::string recvBuffer) {
  _bytesRead = 0;
  bool err = false;
  int responseStatus;
  unsigned int bufferLen = recvBuffer.length();
  while (_bytesRead < bufferLen) {
    if (_request.parseHttpRequest(recvBuffer, _bytesRead) == 1) {
      closeConnection();
      return 1;
    }
    if (_request.parsingDone() == false) {
      return 0;
    }
    _bytesRead += _request.getBytesRead();
    // _request.print();
    if (_request.parseURIContent() == 1) {
      closeConnection();
      return 1;
    }
    if (_CGI.isCGIRequest(_request)) {
      std::cout << "==> found a CGI request\n";
      doCGI();
      _request.reset();
      _response.reset();
      continue;
    }
    responseStatus = _response.build(_request);
    if (responseStatus == 1)
      err = true;
    const char* response = _response.getResponse();
    // std::cout << "response:\n" << response << std::endl;
    if (send(_fd, response, strlen(response), 0) ==
        -1) // TODO how should we protect here? cut client/close server?
        return 1;
    std::vector<char> responseBody = _response.getResponseBody();
    if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1)
        return 1;
    _request.reset();
    _response.reset();
    if (err == true)
      return 1;
    // std::cout << "SUCCESS\n";
  }
  return 0;
}
