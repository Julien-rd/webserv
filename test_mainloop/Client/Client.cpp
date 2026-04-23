#include "Client.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "../CGI/CGI.hpp"
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

Client::Client(): _fd(-1), _request(), _cgi(_request)  { std::cout << "default constructor called\n"; }

#include <sys/socket.h>
#include <sys/types.h>

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

void	Client::handleCGI(void) {
	// bool err = false;

	try {
		_cgi.initCGI();
		_cgi.pipeIO();
		_cgi.spawnProcess();
		_cgi.wait();
		// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
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

int Client::loop(std::string input) {
  _bytesRead = 0;
  bool err = false;
  while (_bytesRead < input.length()) {
    if (_request.parseHttpRequest(input, _bytesRead) == 1)
      return closeConnection();
    if (_request.parsingDone() == false)
      return 0;
    _bytesRead += _request.getBytesRead();
	// if (_cgi.validateRequest()) {
	// 	std::cout << "handling CGI" << std::endl;
	// 	handleCGI();
	// 	return 0;
	// }
    if (_response.build(_request) == 1)
      err = true;
    const char *response = _response.getResponse();
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
    std::cout << "SUCCESS\n";
  }
  return 0;
}
