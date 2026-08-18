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
    _CGI.init(&_request, _fd, _epfd, _sid, &_config->servers.at(_sid));
    setLastActivity();
}

int Client::prepareSendCGI(int pipeReadFd) {
    int status = _CGI.buildResponse(pipeReadFd);
    if (status == RESPONSE_PENDING)
        return RESPONSE_PENDING;
    if (status == RESPONSE_READY) {
        _fullResponse = _CGI.getResponse().getFullResponse();
        _responseSize = _fullResponse.size();
        updateEpoll(EPOLLIN | EPOLLOUT);
    } else if (status == RESPONSE_ERR) {
        // fix: error message?
    }
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

clientStatus Client::closeConnection(int reason) {
    _response.setConnection(false);
    if (reason == CLOSE_CLIENT_ERROR || reason == CLOSE_SERVER_ERROR)
        _response.build(_request);
    _fullResponse = _response.getFullResponse();
    _responseSize = _fullResponse.size();
    updateEpoll(EPOLLIN | EPOLLOUT);
    return CLIENT_RESPONSE_READY;
}

clientStatus Client::sendResponse() {
    if (_fullResponse.empty())
        return CLIENT_KEEP;
    ssize_t n = send(_fd, &_fullResponse[_bytesSent], _responseSize - _bytesSent, 0);
    if (n == -1) {
        closeConnection(CLOSE_TRANSPORT_FAIL);
        return CLIENT_CLOSE;
    }
    _bytesSent += n;
    if (_bytesSent != _responseSize)
        return CLIENT_KEEP;
    if (_response.keepConnection() == false)
        return CLIENT_CLOSE;
    updateEpoll(EPOLLIN);
    _bytesSent = 0;
    _responseSize = 0;
    _response.reset();
    return CLIENT_KEEP;
}

void Client::updateEpoll(const unsigned int &event) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = _fd;
    epoll_ctl(_epfd, EPOLL_CTL_MOD, _fd, &ev);
}

clientStatus Client::parsePending() {
    if (_responseSize > 0)
        return CLIENT_RESPONSE_READY;
    if (recvBufferIsParsed() == true)
        return CLIENT_KEEP;
    if (_request.parseHttpRequest(_recvBuffer, _bytesRead) == 1) {
        if (_request.getStatusCode() == 0)
            _request.setStatusCode(400);
        return closeConnection(CLOSE_CLIENT_ERROR);
    }
    if (_request.parsingDone() == false)
        return CLIENT_KEEP;
    _bytesRead = _request.getBytesRead();
    if (_request.parseURIContent() == 1)
        return closeConnection(CLOSE_CLIENT_ERROR);
    if (_CGI.isCGIRequest(_request)) {
        if (!_CGI.handleCGI())
            return closeConnection(CLOSE_SERVER_ERROR);
        //     _request.reset();  // fix: maybe unnecessary
        //     return CLOSE;
        _request.reset();
        return CLIENT_KEEP;
    }
    _response.build(_request);
    _fullResponse = _response.getFullResponse();
    _responseSize = _fullResponse.size();
    _request.reset();
    if (_bytesRead > _recvBuffer.size() / 2) {
        _recvBuffer.erase(0, _bytesRead);
        _bytesRead = 0;
    }
    updateEpoll(EPOLLIN | EPOLLOUT);
    return CLIENT_RESPONSE_READY;
}

clientStatus Client::parseRecvBuffer(std::string &recvBuffer) {
    _recvBuffer += recvBuffer;
    return parsePending();
}

bool Client::recvBufferIsParsed() const { return _recvBuffer.size() == _bytesRead; }
