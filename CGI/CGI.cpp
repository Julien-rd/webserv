#include "CGI.hpp"

#include "../Client/HttpRequest/HttpRequest.hpp"
#include "../Logger/Logger.hpp"
#include "../Utils/Macros.hpp"
#include "CGIResponse.hpp"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

uint32_t nextCGIIdentifier = 0;

CGI::CGI() {
    _pipeFd[0] = -1;
    _pipeFd[1] = -1;
    _postPipeFd[0] = -1;
    _postPipeFd[1] = -1;
}

// CGI::CGI(const CGI &obj)
//         : _request(obj._request)
//         , _epfd(obj._epfd)
//         , _clientFd(obj._clientFd)
//         , _cgiConfigs(obj._cgiConfigs)
//         , _serverConfig(obj._serverConfig) {
//     _pipeFd[0] = obj._pipeFd[0];
//     _pipeFd[0] = obj._pipeFd[0];
//     _postPipeFd[1] = obj._postPipeFd[1];
//     _postPipeFd[1] = obj._postPipeFd[1];
//     _knownExtensions.clear();
//     for (size_t i = 0; i < obj._knownExtensions.size(); i++) {
//         _knownExtensions.push_back(obj._knownExtensions.at(i));
//     }
// }

CGI::~CGI(void) {
    // if (_pipeFd[0] > -1) {
    // 	close(_pipeFd[0]); // TODO we still need to close thoose fds
    // somewhere
    // }
    // if (_pipeFd[1] > -1) {
    // 	close(_pipeFd[1]);
    // }
}

const CGI &CGI::operator=(const CGI &obj) {
    if (&obj == this)
        return *this;
    _scriptName = obj._scriptName;
    _meta = obj._meta;
    _pid = obj._pid;
    _pipeFd[0] = obj._pipeFd[0];
    _pipeFd[0] = obj._pipeFd[0];
    _postPipeFd[1] = obj._postPipeFd[1];
    _postPipeFd[1] = obj._postPipeFd[1];
    _epfd = obj._epfd;
    _executable = obj._executable;
    _argv[0] = obj._argv[0];
    _argv[1] = obj._argv[1];
    _argv[2] = obj._argv[2];
    for (size_t i = 0; i < 20; i++) {
        _envp[i] = obj._envp[i];
    }
    return *this;
}

// pid_t	CGI::getPid(void) const {
// 	return pid;
// }

// void	CGI::setPid(pid_t pid) {
// 	pid = pid;
// }

bool CGI::scriptFileExists(void) const {
    std::string scriptPath(ROOT_FOLDER);
    scriptPath += _serverConfig->locations.at(0).root + _request->getUriData().path;
    struct stat data;
    if (stat(scriptPath.c_str(), &data) == -1) {
        log(Level::WARNING, "couldn't access CGI script file");
        return false;
    }
    if (data.st_mode & S_IXUSR) {
        return true;
    }
    log(Level::WARNING, "CGI script is not executable\n");
    return false;
}

bool CGI::initCGI(void) {  // fix: would be nice to add a little map with extensions to script type
                           // to make code more flexible
    if (_request->getUriData().extension == ".py") {
        if (!initScript(PYTHON))
            return false;
    } else if (_request->getUriData().extension == ".php") {
        if (!initScript(PHP))
            return false;
    } else {
        log(Level::WARNING, "initialized CGI with unknown extension");
        return false;
    }
    return true;
}

bool CGI::pipeIO(void) {  // Fix: This might leak. Make smart adjustments to if/else to execute
                          // functions that dont depend on each other
    if (pipe(_pipeFd) == -1) {
        log(Level::WARNING, "CGI pipe failed");
        return false;
    }
    if (fcntl(_pipeFd[0], F_SETFD, FD_CLOEXEC) == -1) {
        log(Level::WARNING, "CGI fcntl failed");
        return false;
    }
    if (fcntl(_pipeFd[0], F_SETFL, O_NONBLOCK) == -1) {
        log(Level::WARNING, "CGI fcntl failed");
        return false;
    }
    if (fcntl(_pipeFd[1], F_SETFD, FD_CLOEXEC) == -1) {
        log(Level::WARNING, "CGI fcntl failed");
        return false;
    }
    if (fcntl(_pipeFd[1], F_SETFL, O_NONBLOCK) == -1) {
        log(Level::WARNING, "CGI fcntl failed");
        return false;
    }
    if (_request->getMethod() == "POST") {
        if (_readRegisteredFd != -1) {
            epoll_ctl(_epfd, EPOLL_CTL_DEL, _readRegisteredFd, NULL);
            close(_readRegisteredFd);
            _readRegisteredFd = -1;
        }
        if (_postRegisteredFd != -1) {
            epoll_ctl(_epfd, EPOLL_CTL_DEL, _postRegisteredFd, NULL);
            close(_postRegisteredFd);
            _postRegisteredFd = -1;
        }
        if (pipe(_postPipeFd) == -1) {
            log(Level::WARNING, "CGI post pipe failed");
            return false;
        }
        if (fcntl(_postPipeFd[0], F_SETFD, FD_CLOEXEC) == -1) {
            log(Level::WARNING, "CGI fcntl failed");
            return false;
        }
        // if (fcntl(_postPipeFd[0], F_SETFL, O_NONBLOCK) == -1) {
        //     log(Level::WARNING, "CGI fcntl failed");
        //     return false;
        // }
        if (fcntl(_postPipeFd[1], F_SETFD, FD_CLOEXEC) == -1) {
            log(Level::WARNING, "CGI fcntl failed");
            return false;
        }
        if (fcntl(_postPipeFd[1], F_SETFL, O_NONBLOCK) == -1) {
            log(Level::WARNING, "CGI fcntl failed");
            return false;
        }
    }
    return true;
}

void CGI::flushWriteBuffer(void) {
    ssize_t n = write(_postPipeFd[1], _writeTotal.data() + _writeOffset, _writeTotal.size() - _writeOffset);
    if (n <= 0)
        return ;
    _writeOffset += static_cast<size_t>(n);
    time(&_lastProgressTime);
    if(_writeOffset < _writeTotal.size())
        return ;
    epoll_ctl(_epfd, EPOLL_CTL_DEL, _postPipeFd[1], NULL);
    close(_postPipeFd[1]);
    _postRegisteredFd = -1;
    _writeOffset = 0;
    _writeTotal.clear();
    return;
}

bool CGI::spawnProcess(void) {
    _pid = fork();
    if (_pid == -1) {
        log(Level::WARNING, "fork() failed for post pipe");
        return false;
    }
    if (_pid == 0) {
        if (_request->getMethod() == "POST") {
            if (dup2(_postPipeFd[0], STDIN_FILENO) == -1) {
                log(Level::WARNING, "dup2() failed for post pipe");
                return false;  // fix: here and in redirectIO return in child?
            }
            if (_postPipeFd[0] != -1)
                close(_postPipeFd[0]);
            if (_postPipeFd[1] != -1)
                close(_postPipeFd[1]);
        }
        if (_pipeFd[0] != -1)
            close(_pipeFd[0]);
        if (_epfd != -1)
            close(_epfd);
        if (_clientFd != -1)
            close(_clientFd);
        redirectIO();
        execute();
    } else {
        if (_pipeFd[1] != -1)
            close(_pipeFd[1]);
        if (!addPipeToEpoll())
            return false;
        if (_request->getMethod() == "POST") {
            struct epoll_event ev;
            ev.events = EPOLLOUT;
            ev.data.u64 = (static_cast<uint64_t>(_CGIIdentifier) << 32) |
                          (static_cast<uint64_t>(_clientFd) << 16) |
                          static_cast<uint64_t>(_postPipeFd[1]);
            epoll_ctl(_epfd, EPOLL_CTL_ADD, _postPipeFd[1], &ev);  // fix: this can fail what to do?
            _postRegisteredFd = _postPipeFd[1];
            _writeOffset = 0;
            _writeTotal = _request->getBody();
            time(&_lastProgressTime);
            close(_postPipeFd[0]);
        }
    }
    return true;
}

bool CGI::addPipeToEpoll(void) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    // fix: we have to make sure here that fds are never above the limit 65500 something
    ev.data.u64 = (static_cast<uint64_t>(_CGIIdentifier) << 32) |
                  (static_cast<uint64_t>(_clientFd) << 16) | static_cast<uint64_t>(_pipeFd[0]);
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, _pipeFd[0], &ev) == -1) {
        log(Level::WARNING, "addPipeToEpoll() failed in CGI");
        return false;
    }
    _readRegisteredFd = _pipeFd[0];
    return true;
}

bool CGI::redirectIO(void) {

    if (dup2(_pipeFd[1], STDOUT_FILENO) == -1) {
        log(Level::WARNING, "dup2() failed in CGI");
        return false;
    }
    close(_pipeFd[1]);
    return true;
}

void CGI::execute(void) {
    char *argv[3];
    argv[0] = &_argv[0][0];
    argv[1] = &_argv[1][0];
    argv[2] = NULL;
    if (execve(_executable.c_str(), argv, const_cast<char **>(_envp)) == -1) {
        log(Level::WARNING, "execve() failed in CGI");

        _exit(1);
    }
}

bool CGI::isCGIRequest(const HttpRequest &request) {
    const std::string &uri = request.getUri();
    size_t             queryPos = uri.find('?');
    size_t             pathLen = (queryPos == std::string::npos) ? uri.size() : queryPos;

    size_t dotPos = uri.rfind('.', pathLen - 1);
    if (dotPos == std::string::npos) {
        return false;
    }

    for (size_t i = 0; i < _knownExtensions.size(); i++)
        if (request.getUriData().extension == _knownExtensions[i])
            return true;
    return false;
}

void CGI::init(HttpRequest *request, int clientFd, int epfd, int sid, const t_config *config) {

    _CGIResponse.init(0, config, sid);
    if (++nextCGIIdentifier == 0)
        ++nextCGIIdentifier;
    if (nextCGIIdentifier > 65535)
        nextCGIIdentifier = 1;
    _CGIIdentifier = nextCGIIdentifier;
    _readRegisteredFd = -1;
    _postRegisteredFd = -1;
    _request = request;
    _epfd = epfd;
    _sid = sid;
    _clientFd = clientFd;
    _cgiConfigs = &config->servers.at(sid).cgiConfigs;
    _serverConfig = &config->servers.at(sid);
    // TODO this can be better moved to Server class
    if (!_knownExtensions.size())
        for (size_t i = 0; i < _cgiConfigs->size(); ++i)
            _knownExtensions.push_back(_cgiConfigs->at(i).extension);
}

void CGI::reset(void) {
    // _clientFd = -1; //fix: was buggy but where does this happen instead or does it just get
    // overwritten anyways
    _CGIResponseLen = 0;
    _CGIIdentifier = 0;
    _writeOffset = 0;
    _writeTotal.clear();
    _CGIResponse.reset();
    _CGIResponseStr.erase();
    _CGIResponseStream.erase();
    if (_readRegisteredFd != -1) {
        epoll_ctl(_epfd, EPOLL_CTL_DEL, _readRegisteredFd, NULL);
        close(_readRegisteredFd);
        _readRegisteredFd = -1;
    }
    if (_postRegisteredFd != -1) {
        epoll_ctl(_epfd, EPOLL_CTL_DEL, _postRegisteredFd, NULL);
        close(_postRegisteredFd);
        _postRegisteredFd = -1;
    }
    _scriptName.erase();
    _executable.erase();
    _argv.clear();
    ;
}

unsigned int CGI::getIdentifier(void) const { return _CGIIdentifier; }
int          CGI::getReadFd(void) const { return _readRegisteredFd; }
int          CGI::getPostFd(void) const { return _postRegisteredFd; }
pid_t        CGI::getPid(void) const { return _pid; }
void         CGI::setReadFd(int fd) { _readRegisteredFd = fd; }
void         CGI::setPostFd(int fd) { _postRegisteredFd = fd; }

// int CGI::getPipeFd(void) const { return _pipeFd[0]; }

bool CGI::doCGI(void) {
    if (!scriptFileExists()) {
        return false;
    }
    if (!initCGI()) {
        return false;
    }
    if (!pipeIO()) {
        return false;
    }
    if (!spawnProcess()) {
        return false;
    }
    _CGIPid = getPid();  // oops
    return true;
}

bool CGI::handleCGI() {
    std::stringstream ss;
    ss << "Server " << _sid << " CGI execution " << _request->getUri() << " ";
    log(Level::INFO, ss.str());
    if (!doCGI()) {
        _request->setStatusCode(500);
        return false;
    }
    _CGIResponseStream.erase();
    _CGIResponseLen = 0;
    return true;
}

const CGIResponse &CGI::getResponse() { return _CGIResponse; }

void CGI::errHandler(int fd, errPosition pos) {
    _request->setStatusCode(500);
    _CGIResponse.setConnection(false);
    _CGIResponse.build(*_request);
    if (pos == BEFORE_EPOLL)
        epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

int CGI::buildResponse(int pipeReadFd) {

    std::string buf(BUFFER_SIZE, '\0');
    ssize_t     bytesRead;

    bytesRead = read(pipeReadFd, &buf[0], BUFFER_SIZE - 1);
    if (bytesRead == -1) {
        log(Level::WARNING, "read() failed in Client::handleCGIResponse()");
        errHandler(pipeReadFd, BEFORE_EPOLL);

        return RESPONSE_ERR;
    } else if (bytesRead == 0) {
        int res = waitpid(_CGIPid, NULL, WNOHANG);
        if (res == -1) {
            log(Level::WARNING, "waitpid() failed in Client::handleCGIResponse()");
            errHandler(pipeReadFd, BEFORE_EPOLL);
            return RESPONSE_ERR;
        }
        if (res == 0) {  // fix_CGI: delete this waitpid
            kill(_CGIPid, SIGKILL);
            waitpid(_CGIPid, NULL, 0);
        }
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, pipeReadFd, NULL) == -1) {
            log(Level::WARNING, "epoll_ctl() DEL failed in readCGIPipe()");
            errHandler(pipeReadFd, EPOLL);
            return RESPONSE_ERR;
        }
        close(pipeReadFd);
        _readRegisteredFd = -1;  // fix_CGI: _readReistered reset is this correct epoll style?
        _CGIResponse.setCGIResponseStr(_CGIResponseStream);
        _CGIResponse.setCGIResponseLen(_CGIResponseLen);
        _CGIResponse.setConnection(true);  // fix: necessary?
        _CGIResponse.build(*_request);
        return RESPONSE_READY;
    } else {
        buf.resize(bytesRead);
        _CGIResponseLen += bytesRead;
        _CGIResponseStream.append(buf.data(), bytesRead);
        return RESPONSE_PENDING;
    }
}
