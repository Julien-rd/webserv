#include "Client.hpp"

#include "../CGI/CGI.hpp"
#include "../CGI/CGIResponse.hpp"
#include "../Utils/Macros.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <linux/close_range.h>
#include <netinet/in.h>
#include <poll.h>
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

// Client::Client(const t_config &config, const int sid)
//         : _fd(-1)
//         , _sid(sid)
//         , _epfd(-1)
//         , _request(config.servers.at(sid).clientMaxBody)
//         , _response(config, sid)
//         , _CGIResponseLen(0)
//         , _CGIPid(-1)
//         , _CGIResponse(_CGIResponseStream, _CGIResponseLen, config, sid)
//         , _config(config)
//         , _CGI(
//               _request, _fd, _epfd, _config.servers.at(_sid),
//               _config.servers.at(_sid).cgiConfigs) {
// }

// Client::Client(int epfd, const t_config &config, const int sid)
//         : _fd(-1)
//         , _sid(sid)
//         , _epfd(epfd)
//         , _request()
//         , _response(config, sid)
//         , _CGIResponseLen(0)
//         , _CGIPid(-1)
//         , _CGIResponse(_CGIResponseStream, _CGIResponseLen, config, sid)
//         , _config(config)
//         , _CGI(
//               _request, _fd, _epfd, _config.servers.at(_sid),
//               _config.servers.at(_sid).cgiConfigs) {
// }

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
        std::cerr << "read() failed in Client::handleCGIResponse(): " << strerror(errno) << "\n";
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
        std::cerr << "read() failed in Client::handleCGIResponse(): " << strerror(errno) << "\n";
        return;  // NOTFINISHED: i have no idea whats open here and what this function is
                 // responsible for
    }
    if (bytesRead == 0) {
        int res = waitpid(_CGIPid, NULL, WNOHANG);
        if (res == -1) {
            std::cerr << "waitpid() failed in handleCGIResponse(): " << strerror(errno) << "\n";
            return;  // NOTFINISHED: i have no idea whats open here and what this function is
                     // responsible for // needs to have the epoll del everywhere
        }
        if (res == 0) {  // TODO this branch is untested
            kill(_CGIPid, SIGKILL);
            waitpid(_CGIPid, NULL, 0);
        }
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, pipeReadFd, NULL) == -1) {
            std::cerr << "epoll_ctl() DEL failed in readCGIPipe(): " << strerror(errno) << "\n";
            return;
        }
        close(pipeReadFd);
        // std::cout << "\nbuilding HttpResponse from CGI Response:\n{\n"
        //           << _CGIResponseStream.str() << "\n}\n";
        _CGIResponse.setCGIResponseStr(_CGIResponseStream);
        _CGIResponse.setCGIResponseLen(_CGIResponseLen);
        _CGIResponse.build(_request);
        const char *response = _CGIResponse.getResponse();
        // std::cout << "\nHttpResponse Response:\n" << response << "]" <<std::endl;
        if (send(_fd, response, strlen(response), 0) == -1) {
            std::cerr << "send() failed in handleCGIResponse(): " << strerror(errno) << "\n";
            return;  // NOTFINISHED: i have no idea whats open here and what this function is
                     // responsible for
        }
        std::vector<char> responseBody = _CGIResponse.getResponseBody();
        // for (unsigned int i = 0; i < responseBody.size(); ++i) {
        //     std::cout << responseBody.at(i);
        // }
        // std::cout << std::endl;
        if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1) {
            std::cerr << "epoll_ctl() DEL failed in handleCGIResponse(): " << strerror(errno)
                      << "\n";
            return;  // NOTFINISHED: i have no idea whats open here and what this function is
                     // responsible for
        }
        _request.reset();
        _CGIResponse.reset();
        _CGI.reset();
        _CGIResponseStream.erase();
        _CGIResponseLen = 0;
    } else {
        // std::cout << "adding (( " << buf << " )) to _CGIResponseStream\n";
        buf.resize(bytesRead);
        _CGIResponseLen += bytesRead;
        _CGIResponseStream.append(buf.data(), bytesRead);
        // std::cout << "_CGIResponseStream becamse: ((" << _CGIResponseStream
        //           << " ))" << std::endl;
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
            if(_request.getStatusCode()== 0)
                _request.setStatusCode(400);
            closeConnection(CLOSE_CLIENT_ERROR);
            return CLOSE;
        }
        if (_request.parsingDone() == false)
            return KEEP;
        _bytesRead += _request.getBytesRead();
        // _request.print();
        if (_request.parseURIContent() == 1) {
            closeConnection(CLOSE_CLIENT_ERROR);
            return CLOSE;
        }
        if (_CGI.isCGIRequest(
                _request)) {  // fix: rework this or put inside function if all of these necessary
            std::cout << "==> found a CGI request\n";
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
        // std::cout << "response:\n" << response << std::endl;
        if (send(_fd, response, strlen(response), 0) == -1) {
            closeConnection(CLOSE_TRANSPORT_FAIL);
            return CLOSE;
        }
        std::vector<char> responseBody = _response.getResponseBody();
        if (send(_fd, &responseBody[0], responseBody.size(), 0) == -1) {
            closeConnection(CLOSE_TRANSPORT_FAIL);
            return CLOSE;
        }
        _request.reset();
        _response.reset();
    }
    return KEEP;
}
