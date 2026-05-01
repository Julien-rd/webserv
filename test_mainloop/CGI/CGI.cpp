#include "CGI.hpp"
#include "../Client/HttpRequest/HttpRequest.hpp"
#include <iostream>

#include <cstdio>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const std::string CGI::pythonScriptName = "/python.py";
const std::string CGI::phpScriptName = "/php.php";

CGI::CGI(const HttpRequest &request, int clientFd, int epfd)
    : request(request), epfd(epfd), clientFd(clientFd) {
  this->pipefd[0] = -1;
  this->pipefd[1] = -1;
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

const CGI &CGI::operator=(const CGI &obj) {
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

void CGI::initCGI(void) {
  // this->scriptName = getScriptName(request._uri, this->pythonScriptName,
  // this->phpScriptName);
  if (this->request._uri.compare(0, this->pythonScriptName.size(),
                                 this->pythonScriptName) == 0) {
    initPythonScript();
  } else if (this->request._uri.compare(0, this->pythonScriptName.size(),
                                        this->pythonScriptName) == 0) {
    initPhpScript();
  } else {
    std::cerr << "shouldn't reach here" << std::endl;
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
    throw std::runtime_error("CGI fork failed");
  }
  if (this->pid == 0) {
    this->addPipeToEpoll();
    // std::cout << "========= addPipeToEpoll() succeeded\n";
    this->redirectIO();
    // std::cerr << "========= redirectIO() succeeded\n";
    this->execute();
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
  reinterpret_cast<int *>(&u64)[0] = this->pipefd[0];
  reinterpret_cast<int *>(&u64)[1] =
      this->clientFd; // TODO do we need to set all of this to null if the
                      // clients disconnects?
  ev.data.u64 = u64;
  if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, this->pipefd[0], &ev) == -1) {
    // std::cerr << errno_name(errno) << ": " << strerror(errno) << std::endl;
    throw std::runtime_error("addPipeToEpoll() failed");
  }
}

void CGI::redirectIO(void) {

  // std::cout << "redirectIO(): this->pipefd[1]: " << this->pipefd[1] << " ==
  // this->pipefd[0]: " << this->pipefd[0] << std::endl;
  if (dup2(this->pipefd[1], STDOUT_FILENO) == -1) {
    throw CGI::StandardException();
  }
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
  std::cerr << "executing CGI" << std::endl;
  // int fd = open("cgi_output.txt", O_CREAT | O_NONBLOCK | O_RDWR, 0777);
  // dup2(fd, STDOUT_FILENO);
  // std::cout << "executable is (" << this->executable << ") argv[0] is (" <<
  // this->argv[0] << ")" << std::endl; read(STDIN_FILENO, buf, ) printf(">>>>>
  // path(%s) argv(%s && %s)\n", this->executable, this->argv[0],
  // this->argv[1]); fflush(stdout); std::cout << "caling execve with
  // parameters, path:\n" << this->executable << " ================\n" <<
  // this->argv[0] << "------" << this->argv[1] << "================\n" <<
  // this->envp[0] << std::endl;
  if (execve(this->executable, this->argv, const_cast<char **>(this->envp)) ==
      -1) {
    std::cerr << "execve() failed. shouldn't reach here, maybe invalid "
                 "arguments (path or argv))"
              << std::endl;
    _exit(1);
  }
}

bool CGI::isCGIRequest(const HttpRequest &request) {
  // std::cout << "calling isCGIRequest(): request._uri is: (" << request._uri
  // << ")" << " pythonScriptName is: (" << pythonScriptName << ") " <<
  // std::endl;
  if (request._uri.compare(0, CGI::pythonScriptName.size(),
                           CGI::pythonScriptName) == 0 ||
      request._uri.compare(0, CGI::phpScriptName.size(), CGI::phpScriptName) ==
          0) { // TODO Or could replace this with a dynamic array of known
               // scripts and check if URI matches one of them, then set a
               // variable indicating that we will work with this specifi script
               // for the rest of the execution oF CGI
    return true;
  }
  std::cout << "URI doesn't contain a known script" << std::endl;
  return false;
}

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
