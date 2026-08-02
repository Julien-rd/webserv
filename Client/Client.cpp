#include "Client.hpp"

#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
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
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

Client::Client() {
    _fd = -1;
    _CGIResponseLen = 0;
    _CGIPid = -1;
}

void Client::init(int epfd, const t_config *config, const int sid, const int clientFd) {
    _config = config;
    _epfd = epfd;
    _sid = sid;
    _fd = clientFd;
    _request.init(config->servers.at(sid).clientMaxBody);
    _response.init(config, sid);
    _CGIResponse.init(_CGIResponseLen, config, sid);
    _CGI.init(&_request, _fd, _epfd, &_config->servers.at(_sid));
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

void Client::reset() {  // Fix: maybe even add _cgi.reset? why is responsestream and cgiresponselen
                        // taken to client??
    _fd = -1;
    _CGIResponseStream.erase();  // Fix: find a better way to reset the cgi
    _CGIResponseLen = 0;
    _request.reset();
    _response.reset();
    _CGIResponse.reset();
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

void Client::readCGIPipe(
    int pipeReadFd) {  // ALL OF THE ERRORS HERE CAUSE INFINITE LOADING AND CRASH THE SERVER

    std::string buf(BUFFER_SIZE, '\0');
    ssize_t     bytesRead;

    bytesRead = read(pipeReadFd, &buf[0], BUFFER_SIZE - 1);
    if (bytesRead == -1) {
        _CGIResponseLen = 0;
        _CGIResponseStream.erase();
        log(Level::WARNING, "read() failed in Client::handleCGIResponse()");
        return;  // NOTFINISHED: i have no idea whats open here and what this function is
                 // responsible for
    }
    if (bytesRead == 0) {
        int res = waitpid(_CGIPid, NULL, WNOHANG);
        if (res == -1) {
            log(Level::WARNING, "waitpid() failed in Client::handleCGIResponse()");
            return;  // NOTFINISHED: i have no idea whats open here and what this function is
                     // responsible for // needs to have the epoll del everywhere
        }
        if (res == 0) { 
            kill(_CGIPid, SIGKILL);
            waitpid(_CGIPid, NULL, 0);
        }
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, pipeReadFd, NULL) == -1) {
            log(Level::WARNING, "epoll_ctl() DEL failed in readCGIPipe()");
            return;
        }
        close(pipeReadFd);
        _CGIResponse.setCGIResponseStr(_CGIResponseStream);
        _CGIResponse.setCGIResponseLen(_CGIResponseLen);
        _CGIResponse.build(_request);
        const char *response = _CGIResponse.getResponse();
        if (send(_fd, response, strlen(response), 0) == -1) {
            log(Level::WARNING, "send() failed in readCGIPipe()");
            return;  // NOTFINISHED: i have no idea whats open here and what this function is
                     // responsible for
        }
        std::vector<char> responseBody = _CGIResponse.getResponseBody();
        if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1) {
            log(Level::WARNING, "send() failed in readCGIPipe()");
            return;  // NOTFINISHED: i have no idea whats open here and what this function is
                     // responsible for
        }
        _request.reset();
        _CGIResponse.reset();
        _CGI.reset();
        _CGIResponseStream.erase();
        _CGIResponseLen = 0;
    } else {
        buf.resize(bytesRead);
        _CGIResponseLen += bytesRead;
        _CGIResponseStream.append(buf.data(), bytesRead);
    }
}

void Client::doCGI(void) {
    // std::cout << " in doCGI() => _config.servers.at(_sid).ip: "
    //           << _config.servers.at(_sid).cgiConfigs.size() << "\n";
    // std::cout << " in doCGI() => _config.servers.at(_sid).port: "
    //           << _config.servers.at(_sid).port << "\n";
    if (!_CGI.scriptFileExists()) {
        _request.setStatusCode(500);
        closeConnection(CLOSE_SERVER_ERROR);
        return;
    }
    if (!_CGI.initCGI()) {
        _request.setStatusCode(500);
        closeConnection(CLOSE_SERVER_ERROR);
        return;
    }
    // std::cout << "========= initCGI() succeeded\n";
    if (!_CGI.pipeIO()) {
        _request.setStatusCode(500);
        closeConnection(CLOSE_SERVER_ERROR);
        return;
    }
    // std::cout << "========= pipeIO() succeeded\n";
    if (!_CGI.spawnProcess()) {
        _request.setStatusCode(500);
        closeConnection(CLOSE_SERVER_ERROR);
        return;
    }
    _CGIPid = _CGI.getPid();
    // std::cout << "========= spawnProcess() succeeded\n";
    // writing to the pipe? std::cout << "========= wait() succeeded\n";
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
        // _request.print();
        if (_request.parseURIContent() == 1) {
            closeConnection(CLOSE_CLIENT_ERROR);
            return CLOSE;
        }
        if (_CGI.isCGIRequest(
                _request)) {  // fix: rework this or put inside function if all of these necessary
            std::stringstream ss;
            ss << "Server " << _sid << " CGI execution " << _request.getUri() <<" ";
            log(Level::INFO, ss.str());
            doCGI();
            _request.reset();
            _response.reset();
            _CGIResponseStream.erase();
            _CGIResponseLen = 0;
            // _CGIResponse.reset(); //fix: those were the issues now pls check what needs to be
            // reset _CGI.reset();
            continue;
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
        if(_response.keepConnection() == false)
            return CLOSE;
        _request.reset();
        _response.reset();
    }
    return KEEP;
}
