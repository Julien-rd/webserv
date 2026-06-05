#include "CGI.hpp"
#include "../Client/HttpRequest/HttpRequest.hpp"
#include <iostream>

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

CGI::CGI(const HttpRequest& request, int clientFd, int epfd,
         const t_server&                  serverConfig,
         const std::vector<t_cgi_config>& cgiConfigs)
    : request(request), epfd(epfd), clientFd(clientFd), cgiConfigs(cgiConfigs),
      serverConfig(serverConfig) {
  this->pipefd[0] = -1;
  this->pipefd[1] = -1;
  // TODO this can be better moved to Server class
  for (size_t i = 0; i < this->cgiConfigs.size(); i++) {
    this->knownExtensions.push_back(this->cgiConfigs.at(i).extension);
  }
}

CGI::CGI(const CGI& obj)
    : request(obj.request), epfd(obj.epfd), clientFd(obj.clientFd),
      cgiConfigs(obj.cgiConfigs), serverConfig(obj.serverConfig) {
  this->pipefd[0] = obj.pipefd[0];
  this->pipefd[1] = obj.pipefd[1];
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
  pipefd[1] = obj.pipefd[1];
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

// bool	CGI::validateRequest(void) const {
// 	if (this->request._uri.compare(0, this->pythonScriptName.size(),
// this->pythonScriptName) == 0
// 		|| this->request._uri.compare(0, this->phpScriptName.size(),
// this->phpScriptName) == 0) { // TODO Or could replace this with a dynamic
// array of known scripts and check if URI matches one of them, then set a
// variable indicating that we will work with this specifi script for the rest
// of the execution oF CGI 		std::cout << "URI doesn't contain a
// known script" << std::endl; 		return true;
// 	}
// 	// TODO Validate minimum requirements needed for CGI execution (maybe
// headers for GET or POST. specific requirements for attributes of
// HttpRequest)??? 	return false;
// }

bool CGI::scriptFileExists(void) const {
  std::string scriptPath = "cgi-bin" + request._uriData.path;
  struct stat data;
  if (stat(scriptPath.c_str(), &data) == -1) {
    std::cerr << "couldn't access CGI script file" << std::endl;
    return false;
  }
  if (data.st_mode & S_IFREG & S_IXUSR) {
    std::cout << "script is executable\n";
    return true;
  }
  std::cout << "script is not executable\n";
  return false;
}

void CGI::initCGI(void) {
  if (request._uriData.extension == ".py") {
    initPythonScript();
    std::cout << "initialized python CGI" << std::endl;
  } else if (request._uriData.extension == ".php") {
    initPhpScript();
    std::cout << "initialized php CGI" << std::endl;
  } else {
    // initUnkownExtension();
    std::cout << "initialized CGI with unknown extension" << std::endl;
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
}

void CGI::spawnProcess(void) {
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
    // std::cerr << "========= redirectIO() succeeded\n";
    this->execute();
  } else {
    if (this->pipefd[1] != -1)
      close(this->pipefd[1]);
    this->addPipeToEpoll();
    // std::cout << "========= addPipeToEpoll() succeeded\n";
  }
}

void CGI::addPipeToEpoll(void) {
  // std::cout << "in addPipeToEpoll(), pipefd[0]: " << pipefd[0] << " ==
  // this->epfd: " << this->epfd << std::endl; std::cout << "in
  // addPipeToEpoll(), pipefd[1]: " << pipefd[1] << " == this->epfd: " <<
  // this->epfd << std::endl;
  struct epoll_event ev;
  ev.events = EPOLLIN;
  uint64_t u64;
  reinterpret_cast<int*>(&u64)[0] = this->pipefd[0];
  reinterpret_cast<int*>(&u64)[1] =
      this->clientFd; // TODO do we need to set all of this to null if the
                      // clients disconnects?
  ev.data.u64 = u64;
  std::cout << "pipefd[0]: " << this->pipefd[0]
            << " clientFd: " << this->clientFd << "\n";
  if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, this->pipefd[0], &ev) == -1) {
    // std::cerr << "rfd: " << this->pipefd[0] << ", wfd: " << this->pipefd[1]
    //           << ", epfd: " << this->epfd << ": " << strerror(errno)
    //           << std::endl;
    throw std::runtime_error("addPipeToEpoll() failed in CGI");
  }
}

void CGI::redirectIO(void) {

  // std::cout << "redirectIO(): this->pipefd[1]: " << this->pipefd[1] << " ==
  // this->pipefd[0]: " << this->pipefd[0] << std::endl;
  if (dup2(this->pipefd[1], STDOUT_FILENO) == -1) {
    throw std::runtime_error("dup2() failed in CGI");
  }
  close(this->pipefd[1]);
  // if (dup2(this->pipefd[0], STDIN_FILENO) == -1) {
  // 	throw CGI::StandardException();
  // }
}

void CGI::wait(void) const {
  if (waitpid(this->pid, NULL, 0) == -1) {
    std::cout << "ERROR: waitpid(): " << strerror(errno)
              << std::endl; // TODO Handle real errors and success waits
  }
}

void CGI::execute(void) {
  // int fd = open("cgi_output.txt", O_CREAT | O_NONBLOCK | O_RDWR, 0777);
  // dup2(fd, STDOUT_FILENO);
  // std::cout << "executable is (" << this->executable << ") argv[0] is (" <<
  // this->argv[0] << ")" << std::endl; read(STDIN_FILENO, buf, )
  // printf(">>>>> path(%s) argv(%s && %s)\n", this->executable,
  // this->argv[0], this->argv[1]); fflush(stdout); std::cout << "caling
  // execve with parameters, path:\n" << this->executable << "
  // ================\n" << this->argv[0] << "------" << this->argv[1] <<
  // "================\n" << this->envp[0] << std::endl;
  char* argv[3];
  argv[0] = &this->argv[0][0];
  argv[1] = &this->argv[1][0];
  argv[2] = NULL;
  // std::cerr << "executing (" << this->executable << ")\nargs:\n("
  //           << this->argv[0] << ")\n(" << this->argv[1] << ")\n";
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
  std::string ext = uri.substr(dotPos, pathLen - dotPos);

  for (size_t i = 0; i < this->knownExtensions.size(); i++) {
    if (ext == this->knownExtensions[i]) {
      this->request = request;
      return true;
    }
  }
  return false;
}

void CGI::setClientFd(const int fd) { this->clientFd = fd; }

void CGI::reset(void) { ; }

pid_t CGI::getPid(void) const { return this->pid; }

// int	main(const int argc, const char **argv, const char **envp) {
// 	(void)argc, void(argv), (void)envp;
// 	std::string	content =
// 	"GET /cgi-bin/php.php/this-is-path-info?key1=valuee HTTP/1.1\r\n"
// 	"Host:localhost:8080\r\n"
// 	"User-Agent:    SuperBrowser/1.0\r\n"
// 	"Accept:\ttext/html\r\n"
// 	"Connection: keep-alive  \r\n"
// 	"X-Empty-Header:\r\n"
// 	"Accept-Language: de\r\n"
// 	"Connection: keep-alive  \r\n"
// 	"Accept-Language: en\r\n"
// 	"Accept-Language            : en\r\n"
// 	"\r\n"
// 	"BODYBODYBODYBODYBODYBODYBODYBODYBODYBODY";

// 	HttpRequest	request;
// 	CGI			cgi;

// 	request.parseHttpRequest(content);
// 	try {
// 		cgi.validateRequest(request);
// 		cgi.initCGI();
// 		cgi.pipeIO();
// 		cgi.spawnProcess();
// 		cgi.wait();
// 		// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
// 	}
// 	catch (std::exception& e) {
// 		std::cerr << e.what() << std::endl;
// 	}
// 	return 0;
// }
