#include "Client.hpp"

#include "../CGI/CGI.hpp"
#include "../Utils/Macros.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


Client::Client() {
    _fd = -1;
}

void Client::init(int epfd, const t_config *config, const int sid, const int clientFd) {
    _config = config;
    _epfd = epfd;
    _sid = sid;
    _fd = clientFd;
    _request.init(config->servers.at(sid).clientMaxBody);
    _response.init(config, sid);
    _CGI.init(&_request, _fd, _epfd, _sid, &_config->servers.at(_sid));
    setLastActivity();
}

// Client::Client(const Client &obj)
//         : _fd(obj._fd)
//         , _sid(obj._sid)
//         , _epfd(obj._epfd)
//         , _request(obj._request)
//         , _response(obj._response)
//         , _CGIResponseLen(obj._CGIResponseLen)
//         , _CGIPid(obj._CGIPid)
//         , _CGIResponse(obj._CGIResponse)
//         , _config(obj._config)
//         , _CGI(obj._CGI) {}

// Client &Client::operator=(const Client &obj) {
//     if (&obj == this) {
//         return *this;
//     }
//     _fd = obj._fd;
//     _sid = obj._sid;
//     _epfd = obj._epfd;
//     _request = obj._request;
//     _CGIResponseLen = obj._CGIResponseLen;
//     _CGIPid = obj._CGIPid;
//     return *this;
// }

void Client::setLastActivity() { time(&_lastActivity); }

time_t Client::getLastActivity() { return _lastActivity; }

int Client::getFd() const { return _fd; }

CGI& Client::getCGI()  { return _CGI; }

void Client::reset() {  // Fix: maybe even add _cgi.reset? why is responsestream and cgiresponselen
                        // taken to client??
    _fd = -1;
    _request.reset();
    _response.reset();
    _CGI.reset();
}

void Client::closeConnection(int reason) {
    if (reason == CLOSE_CLIENT_ERROR || reason == CLOSE_SERVER_ERROR)
        _response.build(_request);
    const char *response = _response.getResponse();
    if (send(_fd, response, strlen(response), 0) == -1)
        log(Level::WARNING, "send() failed in Client::closeConnection");
    // NOTFINISHED: i have no idea whats open here and what this function is responsible for
    return;
}



int Client::loop(std::string &recvBuffer) {
    _bytesRead = 0;
    unsigned int bufferLen = recvBuffer.length();
    while (_bytesRead < bufferLen) {
        if (_request.parseHttpRequest(recvBuffer, _bytesRead) == 1) {
            if (_request.getStatusCode() == 0)
                _request.setStatusCode(400);
            closeConnection(CLOSE_CLIENT_ERROR);
            return CLOSE;
        }
        if (_request.parsingDone() == false)
            return KEEP;
        _bytesRead = _request.getBytesRead();
        if (_request.parseURIContent() == 1) {
            closeConnection(CLOSE_CLIENT_ERROR);
            return CLOSE;
        }
        if (_CGI.isCGIRequest(_request)) {
            if (!_CGI.handleCGI()) {
                closeConnection(CLOSE_SERVER_ERROR);
                _request.reset(); //fix: maybe unnecessary
                return CLOSE;
            }
            _request.reset();
            return KEEP;
        }
        _response.build(_request);
        const char *response = _response.getResponse();
        if (send(_fd, response, strlen(response), 0) == -1) {
            closeConnection(CLOSE_TRANSPORT_FAIL);
            return CLOSE;
        }
        std::vector<char> responseBody = _response.getResponseBody();
        if (!responseBody.empty() && send(_fd, &responseBody[0], responseBody.size(), 0) == -1) {
            closeConnection(CLOSE_TRANSPORT_FAIL);
            return CLOSE;
        }
        if (_response.keepConnection() == false)
            return CLOSE;
        _request.reset();
        _response.reset();
    }
    return KEEP;
}
