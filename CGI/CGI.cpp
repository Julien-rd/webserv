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
#include <stdexcept>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

CGI::CGI(const HttpRequest& request, int clientFd, int epfd,
         const t_server&                  serverConfig,
         const std::vector<t_cgi_config>& cgiConfigs)
    : request(request), epfd(epfd), clientFd(clientFd), cgiConfigs(cgiConfigs),
      serverConfig(serverConfig) {
  this->pipefd[0] = -1;
  this->pipefd[1] = -1;
  this->postPipefd[0] = -1;
  this->postPipefd[1] = -1;
  // TODO this can be better moved to Server class
  for (size_t i = 0; i < this->cgiConfigs.size(); i++) {
    this->knownExtensions.push_back(this->cgiConfigs.at(i).extension);
  }
}

CGI::CGI(const CGI& obj)
    : request(obj.request), epfd(obj.epfd), clientFd(obj.clientFd),
      cgiConfigs(obj.cgiConfigs), serverConfig(obj.serverConfig) {
  this->pipefd[0] = obj.pipefd[0];
  this->pipefd[0] = obj.pipefd[0];
  this->postPipefd[1] = obj.postPipefd[1];
  this->postPipefd[1] = obj.postPipefd[1];
  this->knownExtensions.clear();
  for (size_t i = 0; i < this->cgiConfigs.size(); i++) {
    this->knownExtensions.push_back(this->cgiConfigs.at(i).extension);
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
  scriptName = obj.scriptName;
  meta = obj.meta;
  pid = obj.pid;
  pipefd[0] = obj.pipefd[0];
  pipefd[0] = obj.pipefd[0];
  postPipefd[1] = obj.postPipefd[1];
  postPipefd[1] = obj.postPipefd[1];
  epfd = obj.epfd;
  executable = obj.executable;
  argv[0] = obj.argv[0];
  argv[1] = obj.argv[1];
  argv[2] = obj.argv[2];
  for (size_t i = 0; i < 20; i++) {
    this->envp[i] = obj.envp[i];
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
    scriptPath += "/PasswordManager" + request._uriData.path; //Fix: This is hardcoded, pls add something like locationfind here we want our server adaptable so the config setup matters
   std::cout << scriptPath << "\n"; 
  struct stat data;
  if (stat(scriptPath.c_str(), &data) == -1) {
    std::cerr << "couldn't access CGI script file"
              << std::endl; // TODO handle error pages here too???
    return false;
  }
  if (data.st_mode & S_IXUSR) {
    return true;
  }
  std::cout << "script is not executable\n";
  return false;
}

void CGI::initCGI(void) {
  if (request._uriData.extension == ".py") {
    initPythonScript();
    // std::cout << "initialized python CGI" << std::endl;
  } else if (request._uriData.extension == ".php") {
    initPhpScript();
    // std::cout << "initialized php CGI" << std::endl;
  } else {
    // initUnkownExtension();
    // std::cout << "initialized CGI with unknown extension" << std::endl;
  }
}

void CGI::pipeIO(void) {
  if (pipe(this->pipefd) == -1) {
    throw std::runtime_error("CGI pipe failed");
  }
  if (fcntl(this->pipefd[0], F_SETFD, FD_CLOEXEC) == -1) {
    throw std::runtime_error("CGI fcntl");
  }
  if (fcntl(this->pipefd[0], F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("CGI fcntl");
  }
  if (fcntl(this->pipefd[1], F_SETFD, FD_CLOEXEC) == -1) {
    throw std::runtime_error("CGI fcntl");
  }
  if (fcntl(this->pipefd[1], F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("CGI fcntl");
  }
  if (this->request._method == "POST") {
    if (pipe(this->postPipefd) == -1) {
      throw std::runtime_error("CGI post pipe failed");
    }
    if (fcntl(this->postPipefd[0], F_SETFD, FD_CLOEXEC) == -1) {
      throw std::runtime_error("CGI fcntl");
    }
    if (fcntl(this->postPipefd[0], F_SETFL, O_NONBLOCK) == -1) {
      throw std::runtime_error("CGI fcntl");
    }
    if (fcntl(this->postPipefd[1], F_SETFD, FD_CLOEXEC) == -1) {
      throw std::runtime_error("CGI fcntl");
    }
    if (fcntl(this->postPipefd[1], F_SETFL, O_NONBLOCK) == -1) {
      throw std::runtime_error("CGI fcntl");
    }
  }
}

void CGI::spawnProcess(void) {
  // std::cout << "postpipe[0]: " << this->postPipefd[0] << "\n";
  if (this->request._method == "POST") {
    if (dup2(this->postPipefd[0], STDIN_FILENO) == -1) {
      throw std::runtime_error("dup2 failed for post pipe");
    }
  }
  this->pid = fork();
  if (this->pid == -1) {
    throw std::runtime_error("fork() failed in CGI");
  }
  if (this->pid == 0) {
    if (this->pipefd[0] != -1)
      close(this->pipefd[0]);
    if (this->epfd != -1)
      close(this->epfd);
    if (this->clientFd != -1)
      close(this->clientFd);
    this->redirectIO();
    this->execute();
  } else {
    if (this->pipefd[1] != -1) {
      close(this->pipefd[1]);
    }
    this->addPipeToEpoll();
    if (this->request._method == "POST") {
      std::stringstream ss;
      for (size_t i = 0; i < this->request.getBody().size(); i++) {
        ss << this->request.getBody()[i];
      }
      write(this->postPipefd[1], ss.str().c_str(),
            this->request._contentLength);
      close(this->postPipefd[1]);
      close(this->postPipefd[0]);
    }
  }
}

void CGI::addPipeToEpoll(void) {
  struct epoll_event ev;
  ev.events = EPOLLIN;
  uint64_t u64;
  reinterpret_cast<int*>(&u64)[0] = this->pipefd[0];
  reinterpret_cast<int*>(&u64)[1] =
      this->clientFd; // TODO do we need to set all of this to null if the
                      // clients disconnects?
  ev.data.u64 = u64;
  if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, this->pipefd[0], &ev) == -1) {
    throw std::runtime_error("addPipeToEpoll() failed in CGI");
  }
}

void CGI::redirectIO(void) {

  if (dup2(this->pipefd[1], STDOUT_FILENO) == -1) {
    throw std::runtime_error("dup2() failed in CGI");
  }
  close(this->pipefd[1]);
}

void CGI::wait(void) const {
  if (waitpid(this->pid, NULL, 0) == -1) {
    std::cout << "ERROR: waitpid(): " << strerror(errno)
              << std::endl; // TODO Handle real errors and success waits
  }
}

void CGI::execute(void) {
  char* argv[3];
  argv[0] = &this->argv[0][0];
  argv[1] = &this->argv[1][0];
  argv[2] = NULL;
  if (execve(this->executable.c_str(), argv, const_cast<char**>(this->envp)) ==
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
      this->request = request;
      return true;
    }
  }
  return false;
}

void CGI::setClientFd(const int fd) { this->clientFd = fd; }

void CGI::reset(void) { ; }

pid_t CGI::getPid(void) const { return this->pid; }
