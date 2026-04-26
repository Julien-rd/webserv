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

#include <sys/socket.h>
#include <sys/types.h>

Client::Client(): _fd(-1), _epfd(-1), _request(), _cgi(_request, *this, _epfd) {}

Client::Client(int epfd): _fd(-1), _epfd(epfd), _request(), _cgi(_request, *this, _epfd){}

Client::Client(const Client& obj): _fd(obj._fd), _epfd(obj._epfd), _request(obj._request), _cgi(obj._cgi) {}

const Client&	Client::operator=(const Client& obj) {
	if (&obj == this) {
		return *this;
	}
	_fd = obj._fd;
	_request = obj._request;
	_cgi = obj._cgi;
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

void	Client::handleCGI(CGI& cgi) {
	// bool err = false;

	try {
		cgi.initCGI();
		std::cout << "========= initCGI() succeeded\n";
		cgi.pipeIO();
		std::cout << "========= pipeIO() succeeded\n";
		cgi.spawnProcess();
		std::cout << "========= spawnProcess() succeeded\n";
		cgi.wait();
		std::cout << "========= wait() succeeded\n";
		// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
	}
	catch (std::exception& e) {
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

bool	isCGIRequest(const HttpRequest& request) {
	std::string	pythonScriptName = "/python.py";
	std::string	phpScriptName = "/php.php";
	// std::cout << "calling isCGIRequest(): request._uri is: (" << request._uri << ")" << " pythonScriptName is: (" << pythonScriptName << ") " << std::endl;
	if (request._uri.compare(0, pythonScriptName.size(), pythonScriptName) == 0
		|| request._uri.compare(0, phpScriptName.size(), phpScriptName) == 0) { // TODO Or could replace this with a dynamic array of known scripts and check if URI matches one of them, then set a variable indicating that we will work with this specifi script for the rest of the execution oF CGI
		return true;
	}
	std::cout << "URI doesn't contain a known script" << std::endl;
	return false;
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
	if (isCGIRequest(_request)) {
		CGI	cgi(_request, *this, _epfd);
		handleCGI(cgi);
		return 0;
	}
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
    // std::cout << "SUCCESS\n";
  }
  return 0;
}
