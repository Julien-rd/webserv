#include "CGI.hpp"
#include "../Utils/Macros.hpp"
#include "../Client/HttpRequest/HttpRequest.hpp"

#include <iostream>
#include <sstream>

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

CGI::CGI(HttpRequest& request, int clientFd, int epfd,
         const t_server&                  serverConfig,
         const std::vector<t_cgi_config>& cgiConfigs)
    : _request(request), _epfd(epfd), _clientFd(clientFd), _cgiConfigs(cgiConfigs),
      _serverConfig(serverConfig) {
  this->pipefd[0] = -1;
  this->pipefd[1] = -1;
  this->postPipefd[0] = -1;
  this->postPipefd[1] = -1;
  // TODO this can be better moved to Server class
  for (size_t i = 0; i < this->_cgiConfigs.size(); i++) {
    this->knownExtensions.push_back(this->_cgiConfigs.at(i).extension);
  }
}

CGI::CGI(const CGI& obj)
    : _request(obj._request), _epfd(obj._epfd), _clientFd(obj._clientFd),
      _cgiConfigs(obj._cgiConfigs), _serverConfig(obj._serverConfig) {
  this->pipefd[0] = obj.pipefd[0];
  this->pipefd[0] = obj.pipefd[0];
  this->postPipefd[1] = obj.postPipefd[1];
  this->postPipefd[1] = obj.postPipefd[1];
  this->knownExtensions.clear();
  for (size_t i = 0; i < this->_cgiConfigs.size(); i++) {
    this->knownExtensions.push_back(this->_cgiConfigs.at(i).extension);
  }
}

CGI::~CGI(void) {
  // if (this->pipefd[0] > -1) {
  // 	close(this->pipefd[0]); // TODO we still need to close thoose fds
  // somewhere
  // }
  // if (this->pipefd[1] > -1) {
  // 	close(this->pipefd[1]);
  // }
}

const CGI& CGI::operator=(const CGI& obj) {
  if (&obj == this) {
    return *this;
  }
  _scriptName = obj._scriptName;
  _meta = obj._meta;
  _pid = obj._pid;
  pipefd[0] = obj.pipefd[0];
  pipefd[0] = obj.pipefd[0];
  postPipefd[1] = obj.postPipefd[1];
  postPipefd[1] = obj.postPipefd[1];
  _epfd = obj._epfd;
  _executable = obj._executable;
  _argv[0] = obj._argv[0];
  _argv[1] = obj._argv[1];
  _argv[2] = obj._argv[2];
  for (size_t i = 0; i < 20; i++) {
    this->_envp[i] = obj._envp[i];
  }
  return *this;
}

// pid_t	CGI::getPid(void) const {
// 	return this->pid;
// }

// void	CGI::setPid(pid_t pid) {
// 	this->pid = pid;
// }

bool CGI::scriptFileExists(void) const {
    std::string scriptPath(ROOT_FOLDER);
    scriptPath += "/PasswordManager" + _request._uriData.path; //Fix: This is hardcoded, pls add something like locationfind here we want our server adaptable so the config setup matters
   // std::cout << scriptPath << "\n"; 
  struct stat data;
  if (stat(scriptPath.c_str(), &data) == -1) {
    std::cerr << "couldn't access CGI script file"
              << std::endl; // TODO handle error pages here too???
    return false;
  }
  if (data.st_mode & S_IXUSR) {
    return true;
  }
  std::cerr << "CGI script is not executable\n";
  return false;
}

bool CGI::initCGI(void) {
  if (_request._uriData.extension == ".py") {
    if (!initPythonScript())
        return false;
    // std::cout << "initialized python CGI" << std::endl;
  } else if (_request._uriData.extension == ".php") {
    if (!initPhpScript())
        return false;
    // std::cout << "initialized php CGI" << std::endl;
  } else {
    // initUnkownExtension();
    std::cerr << "initialized CGI with unknown extension" << std::endl;
    return false;
  }
  return true;
}

bool CGI::pipeIO(void) { //Fix: This might leak. Make smart adjustments to if/else to execute functions that dont depend on each other
  if (pipe(this->pipefd) == -1) {
      std::cerr << "CGI pipe failed\n";
      return false;
  }
  if (fcntl(this->pipefd[0], F_SETFD, FD_CLOEXEC) == -1) {
      std::cerr << "CGI fcntl\n";
      return false;
  }
  if (fcntl(this->pipefd[0], F_SETFL, O_NONBLOCK) == -1) {
      std::cerr << "CGI fcntl\n";
      return false;
  }
  if (fcntl(this->pipefd[1], F_SETFD, FD_CLOEXEC) == -1) {
      std::cerr << "CGI fcntl\n";
      return false;
  }
  if (fcntl(this->pipefd[1], F_SETFL, O_NONBLOCK) == -1)  {
      std::cerr << "CGI fcntl\n";
      return false;
  }
  if (this->_request._method == "POST") {
      if (pipe(this->postPipefd) == -1) {
          std::cerr << "CGI post pipe failed\n";
          return false;
      }
    if (fcntl(this->postPipefd[0], F_SETFD, FD_CLOEXEC) == -1)  {
          std::cerr << "CGI fcntl\n";
        return false;
    }
       if (fcntl(this->postPipefd[0], F_SETFL, O_NONBLOCK) == -1) {
          std::cerr << "CGI fcntl\n";
           return false;
       }
    if (fcntl(this->postPipefd[1], F_SETFD, FD_CLOEXEC) == -1) {
          std::cerr << "CGI fcntl\n";
          return false;
    }
    if (fcntl(this->postPipefd[1], F_SETFL, O_NONBLOCK) == -1) {
          std::cerr << "CGI fcntl\n";
          return false;
      }
  }
  return true;
}

bool CGI::spawnProcess(void) {
  // std::cout << "postpipe[0]: " << this->postPipefd[0] << "\n";
  if (this->_request._method == "POST") {
      if (dup2(this->postPipefd[0], STDIN_FILENO) == -1) {
          std::cerr << "dup2 failed for post pipe\n";
          return false;
      }
  }
  this->_pid = fork();
  if (this->_pid == -1) {
      std::cerr << "fork() failed in CGI\n";
      return false;
  }
  if (this->_pid == 0) {
    if (this->pipefd[0] != -1)
      close(this->pipefd[0]);
    if (this->_epfd != -1)
      close(this->_epfd);
    if (this->_clientFd != -1)
      close(this->_clientFd);
    this->redirectIO();
    this->execute();
  } else {
    if (this->pipefd[1] != -1) {
      close(this->pipefd[1]);
    }
    if (!this->addPipeToEpoll())
        return false;
    if (this->_request._method == "POST") {
        const std::vector<char>& body = this->_request.getBody();
      std::string bodyStr(body.begin(), 
                           body.end());
      size_t totalWritten = 0;
      while (totalWritten < this->_request._contentLength)
      {
          ssize_t written = write(
              this->postPipefd[1],
              bodyStr.data() + totalWritten,
              this->_request._contentLength - totalWritten
          );
      
          if (written <= 0)
          {
              // Fix: handle error
              break;
          }
      
          totalWritten += written;
      }
      close(this->postPipefd[1]);
      close(this->postPipefd[0]);
    }
  }
  return true;
}

bool CGI::addPipeToEpoll(void) {
  struct epoll_event ev;
  ev.events = EPOLLIN;
  uint64_t u64;
  reinterpret_cast<int*>(&u64)[0] = this->pipefd[0];
  reinterpret_cast<int*>(&u64)[1] =
      this->_clientFd; // TODO do we need to set all of this to null if the
                      // clients disconnects?
  ev.data.u64 = u64;
  if (epoll_ctl(this->_epfd, EPOLL_CTL_ADD, this->pipefd[0], &ev) == -1) {
      std::cerr << "addPipeToEpoll() failed in CGI\n";
      return false;
  }
  return true;
}

bool CGI::redirectIO(void) {

  if (dup2(this->pipefd[1], STDOUT_FILENO) == -1) {
      std::cerr << "dup2() failed in CGI\n";
      return false;
  }
  close(this->pipefd[1]);
  return true;
}

void CGI::wait(void) const {
  if (waitpid(this->_pid, NULL, 0) == -1) {
    std::cerr << "ERROR: waitpid(): " << strerror(errno)
              << std::endl; // TODO Handle real errors and success waits
  }
}

void CGI::execute(void) {
  char* argv[3];
  argv[0] = &this->_argv[0][0];
  argv[1] = &this->_argv[1][0];
  argv[2] = NULL;
  if (execve(this->_executable.c_str(), argv, const_cast<char**>(this->_envp)) ==
      -1) {
    std::cerr << "execve() failed. shouldn't reach here, maybe invalid "
                 "arguments (path or argv))"
              << std::endl;
    _exit(1);
  }
}

bool CGI::isCGIRequest(const HttpRequest& request) {
  const std::string& uri = request._uri;
  // Strip query string for extension detection
  size_t queryPos = uri.find('?');
  size_t pathLen = (queryPos == std::string::npos) ? uri.size() : queryPos;

  // Find the last '.' in the path portion
  size_t dotPos = uri.rfind('.', pathLen - 1);
  if (dotPos == std::string::npos) {
    return false;
  }

  // Extract the extension including the dot (e.g. ".py", ".pl")
  for (size_t i = 0; i < this->knownExtensions.size(); i++) {
    // std::cout << "comparing " << request._uriData.extension << " with "
    //           << this->knownExtensions[i] << "\n";
    if (request._uriData.extension == this->knownExtensions[i]) {
      this->_request = request;
      return true;
    }
  }
  return false;
}

void CGI::setClientFd(const int fd) { this->_clientFd = fd; }

void CGI::reset(void) { 
    _scriptName.erase();
    _executable.erase();
    _argv.clear();
    ; }

pid_t CGI::getPid(void) const { return this->_pid; }
