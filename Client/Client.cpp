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

Client::Client() { _fd = -1; }

void Client::init(int epfd, const t_config *config, const int sid, const int clientFd) {
    _config = config;
    _epfd = epfd;
    _sid = sid;
    _fd = clientFd;
    _bytesSent = 0;
    _bytesRead = 0;
    _responseSize = 0;
    _request.init(config->servers.at(sid).clientMaxBody);
    _response.init(config, sid);
    _CGI.init(&_request, _fd, _epfd, _sid, config);
    _maxRecvBuffer = config->servers.at(sid).clientMaxBody + HEADER_SLACK;
    setLastActivity();
}

int Client::prepareSendCGI(int pipeReadFd) {
    int status = _CGI.buildResponse(pipeReadFd);
    if (status == RESPONSE_PENDING)
        return RESPONSE_PENDING;
    _fullResponse = _CGI.getResponse().getFullResponse();
    _responseSize = _fullResponse.size();
    updateEpoll(EPOLLIN | EPOLLOUT); //fix: this can fail?
    _CGI.reset();
    return status;
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

CGI &Client::getCGI() { return _CGI; }

void Client::reset() {  // Fix: maybe even add _cgi.reset? why is responsestream and cgiresponselen
                        // taken to client??
    _fd = -1;
    _request.reset();
    _response.reset();
    _CGI.reset();
    _recvBuffer.clear();
    _fullResponse.clear();
    _bytesSent = 0;
    _responseSize = 0;
    _bytesRead = 0;
}

bool Client::closeConnection(int reason) {
    _response.setConnection(false);
    if (reason == CLOSE_CLIENT_ERROR || reason == CLOSE_SERVER_ERROR)
        _response.build(_request);
    _fullResponse = _response.getFullResponse();
    _responseSize = _fullResponse.size();
    if (updateEpoll(EPOLLIN | EPOLLOUT) == false)
        return false;
    return true;
}

bool Client::sendResponse() {
    if (_fullResponse.empty())
        return true;
    ssize_t n = send(_fd, &_fullResponse[_bytesSent], _responseSize - _bytesSent, 0);
    if (n == -1)
        return false;
    _bytesSent += n;
    if (_bytesSent != _responseSize)
        return true;
    if (_response.keepConnection() == false)
        return false;
    _fullResponse.clear();
    _bytesSent = 0;
    _responseSize = 0;
    _request.reset();
    _response.reset();
    return updateEpoll(EPOLLIN);
}

bool Client::updateEpoll(const unsigned int &event) {
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = event;
    ev.data.u64 = 0;
    ev.data.fd = _fd;
    if (epoll_ctl(_epfd, EPOLL_CTL_MOD, _fd, &ev) == -1) {
        log(Level::WARNING, "epoll_ctl MOD failed");
        return false;
    }
    return true;
}

bool Client::parsePending() {
    if (_responseSize > 0 || recvBufferIsParsed() == true)
        return true;
    if (_request.parseHttpRequest(_recvBuffer, _bytesRead) == 1) {
        if (_request.getStatusCode() == 0)
            _request.setStatusCode(400);
        return closeConnection(CLOSE_CLIENT_ERROR);
    }
    _bytesRead = _request.getBytesRead();
    if (_request.parsingDone() == false)
        return true;
    if (_request.parseURIContent() == 1)
        return closeConnection(CLOSE_CLIENT_ERROR);
    if (_CGI.isCGIRequest(_request)) {
        _CGI.init(&_request, _fd, _epfd, _sid, _config);
        if (!_CGI.handleCGI())
            return closeConnection(CLOSE_SERVER_ERROR);
        return true;
    }
    _response.build(_request);
    _fullResponse = _response.getFullResponse();
    _responseSize = _fullResponse.size();
    _request.reset();
    if (_bytesRead > _recvBuffer.size() / 2) {
        _recvBuffer.erase(0, _bytesRead);
        _bytesRead = 0;
    }
    if (updateEpoll(EPOLLIN | EPOLLOUT) == false)
        return false;
    return true;
}

bool Client::parseRecvBuffer(std::string &recvBuffer) {
    if (_maxRecvBuffer - _recvBuffer.size() < recvBuffer.size()) {
        _request.setStatusCode(_request.parsingDone() ? 413 : 431);
        log(Level::INFO, "recvBuffer too big");
        return closeConnection(CLOSE_CLIENT_ERROR);
    }
    _recvBuffer += recvBuffer;
    return parsePending();
}

bool Client::recvBufferIsParsed() const { return _recvBuffer.size() == _bytesRead; }

bool Client::keepConnection() const { return _response.keepConnection(); }
