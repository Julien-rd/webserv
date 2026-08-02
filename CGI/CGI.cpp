#include "CGI.hpp"

#include "../Client/HttpRequest/HttpRequest.hpp"
#include "../Logger/Logger.hpp"
#include "../Utils/Macros.hpp"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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
        // std::cout << "initialized python CGI" << std::endl;
    } else if (_request->getUriData().extension == ".php") {
        if (!initScript(PHP))
            return false;
        // std::cout << "initialized php CGI" << std::endl;
    } else {
        // initUnkownExtension();
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
        if (pipe(_postPipeFd) == -1) {
            log(Level::WARNING, "CGI post pipe failed");
            return false;
        }
        if (fcntl(_postPipeFd[0], F_SETFD, FD_CLOEXEC) == -1) {
            log(Level::WARNING, "CGI fcntl failed");
            return false;
        }
        if (fcntl(_postPipeFd[0], F_SETFL, O_NONBLOCK) == -1) {
            log(Level::WARNING, "CGI fcntl failed");
            return false;
        }
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

bool CGI::spawnProcess(void) {
    // std::cout << "postpipe[0]: " << _postPipeFd[0] << "\n";
    if (_request->getMethod() == "POST") {
        if (dup2(_postPipeFd[0], STDIN_FILENO) == -1) {
            log(Level::WARNING, "dup2() failed for post pipe");
            return false;
        }
    }
    _pid = fork();
    if (_pid == -1) {
        log(Level::WARNING, "fork() failed for post pipe");
        return false;
    }
    if (_pid == 0) {
        if (_pipeFd[0] != -1)
            close(_pipeFd[0]);
        if (_epfd != -1)
            close(_epfd);
        if (_clientFd != -1)
            close(_clientFd);
        redirectIO();
        execute();
    } else {
        if (_pipeFd[1] != -1) {
            close(_pipeFd[1]);
        }
        if (!addPipeToEpoll())
            return false;
        if (_request->getMethod() == "POST") {
            const std::vector<char> &body = _request->getBody();
            std::string              bodyStr(body.begin(), body.end());
            size_t                   totalWritten = 0;
            while (totalWritten < _request->getContentLength()) {
                ssize_t written = write(_postPipeFd[1],
                                        bodyStr.data() + totalWritten,
                                        _request->getContentLength() - totalWritten);
                if (written == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    if (errno == EINTR) {
                        continue;
                    }
                    log(Level::WARNING, "write() failed in CGI");
                    break;
                }
                totalWritten += written;
            }
            close(_postPipeFd[1]);
            close(_postPipeFd[0]);
        }
    }
    return true;
}

bool CGI::addPipeToEpoll(void) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    uint64_t u64;
    reinterpret_cast<int *>(&u64)[0] = _pipeFd[0];
    reinterpret_cast<int *>(&u64)[1] = _clientFd;  // TODO do we need to set all of this to
                                                   // null if the clients disconnects?
    ev.data.u64 = u64;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, _pipeFd[0], &ev) == -1) {
        log(Level::WARNING, "addPipeToEpoll() failed in CGI");
        return false;
    }
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

void CGI::wait(void) const {
    if (waitpid(_pid, NULL, 0) == -1)
        log(Level::WARNING, "waitpid() failed in CGI");
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
    // Strip query string for extension detection
    size_t             queryPos = uri.find('?');
    size_t             pathLen = (queryPos == std::string::npos) ? uri.size() : queryPos;

    // Find the last '.' in the path portion
    size_t dotPos = uri.rfind('.', pathLen - 1);
    if (dotPos == std::string::npos) {
        return false;
    }

    // Extract the extension including the dot (e.g. ".py", ".pl")
    for (size_t i = 0; i < _knownExtensions.size(); i++) {
        // std::cout << "comparing " << request._uriData.extension << " with "
        //           << _knownExtensions[i] << "\n";
        if (request.getUriData().extension == _knownExtensions[i]) {
            _request = &request;
            return true;
        }
    }
    return false;
}

void CGI::init(HttpRequest *request, int clientFd, int epfd, const t_server *serverConfig) {

    _request = request;
    _epfd = epfd;
    _clientFd = clientFd;
    _cgiConfigs = &serverConfig->cgiConfigs;
    _serverConfig = serverConfig;
    // TODO this can be better moved to Server class
    for (size_t i = 0; i < _cgiConfigs->size(); ++i) {
        _knownExtensions.push_back(_cgiConfigs->at(i).extension);
    }
}

void CGI::reset(void) {
    // _clientFd = -1; //fix: was buggy but where does this happen instead or does it just get
    // overwritten anyways
    _scriptName.erase();
    _executable.erase();
    _argv.clear();
    ;
}

pid_t CGI::getPid(void) const { return _pid; }
