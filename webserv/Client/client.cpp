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

void Client::loop(std::string input, int fd) {
  _bytesRead = 0;
  _fd = fd; // implement fd in constructor
  while (_bytesRead < input.length()) {
    if (_request.parseHttpRequest(input, _bytesRead) == 1)
      ;
    if (_request.parsingDone() == false)
      return;
    _bytesRead += _request.getBytesRead();
    if (_request.parsingDone() == true) {
      _request.print();
      _response.build(_request);
      const char *response = _response.getResponse();
      if (send(_fd, response, strlen(response), 0) == -1)
        abort();
      _request.reset();
      _response.reset();
      std::cout << "SUCCESS\n";
    } else
      return;
  }
}