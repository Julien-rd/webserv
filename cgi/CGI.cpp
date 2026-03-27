#include "CGI.hpp"
#include "HttpRequest.hpp"
#include <exception>
#include <iostream>

#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

CGI::CGI(void): envp(new const char*[12]),
pythonScriptName("/cgi-bin/python.py"),
phpScriptName("/cgi-bin/php.php") {
	this->pipefd[0] = -1;
	this->pipefd[1] = -1;
}

CGI::~CGI(void) {
	delete []this->envp;
	if (this->pipefd[0] > -1) {
		close(this->pipefd[0]);
	}
	if (this->pipefd[0] > -1) {
		close(this->pipefd[1]);
	}
}

// pid_t	CGI::getPid(void) const {
// 	return this->pid;
// }

// void	CGI::setPid(pid_t pid) {
// 	this->pid = pid;
// }

void	CGI::validateRequest(const HttpRequest& request) const {
	if (request._uri.compare(0, this->pythonScriptName.size(), this->pythonScriptName)
		&& request._uri.compare(0, this->phpScriptName.size(), this->phpScriptName)) { // TODO Or could replace this with a dynamic array of known scripts and check if URI matches one of them, then set a variable indicating that we will work with this specifi script for the rest of the execution oF CGI
		std::cout << "URI doesn't contain a known script" << std::endl;
		throw CGI::StandardException();
	}
	// TODO Validate minimum requirements needed for CGI execution (maybe headers for GET or POST. specific requirements for attributes of HttpRequest)???
}

void	CGI::initCGI(const HttpRequest& request) {
	this->scriptName = getScriptName(request._uri, this->pythonScriptName, this->phpScriptName);
	if (this->scriptName == this->pythonScriptName) {
		initPythonScript(request);
	}
	else if (this->scriptName == this->phpScriptName) {
		initPhpScript(request);
	}
	else {
		std::cerr << "shouldn't reach here" << std::endl;
	}
}

void	CGI::pipeIO(void) {
	if (pipe(this->pipefd) == -1) {
		throw CGI::StandardException();
	}
	if (fcntl(this->pipefd[0], F_SETFD, O_NONBLOCK) == -1) {
		throw CGI::StandardException();
	}
	if (fcntl(this->pipefd[1], F_SETFD, O_NONBLOCK) == -1) {
		throw CGI::StandardException();
	}
}

void	CGI::spawnProcess(void) {
	this->pid = fork();
	if (this->pid == -1) {
		throw CGI::StandardException();
	}
	if (this->pid == 0) {
		this->execute();
	}
}

void	CGI::wait(void) const {
	if (waitpid(this->pid, NULL, 0) == -1) {
		std::cout << "ERROR: waitpid(): " << strerror(errno) << std::endl; // TODO Handle real errors and success waits
	}
}

void	CGI::execute(void) {
	close(this->pipefd[0]);
	close(this->pipefd[1]);
	// int fd = open("cgi_output.txt", O_CREAT | O_NONBLOCK | O_RDWR, 0777);
	// dup2(fd, STDOUT_FILENO);
	// std::cout << "executable is (" << this->executable << ") argv[0] is (" << this->argv[0] << ")" << std::endl;
	// read(STDIN_FILENO, buf, )
	// printf(">>>>> path(%s) argv(%s && %s)\n", this->executable, this->argv[0], this->argv[1]);
	// fflush(stdout);
	if (execve(this->executable, this->argv, const_cast<char **>(this->envp)) == -1) {
		std::cerr << "execve() failed. shouldn't reach here, maybe invalid arguments (path or argv))" << std::endl;
		throw CGI::StandardException();
	}
}

// void	CGI::redirectIO(void) {

// 	if (dup2(this->pipefd[1], STDOUT_FILENO) == -1) {
// 		throw CGI::StandardException();
// 	}
// 	if (dup2(this->pipefd[0], STDIN_FILENO) == -1) {
// 		throw CGI::StandardException();
// 	}
// }

int	main(const int argc, const char **argv, const char **envp) {
	(void)argc, void(argv), (void)envp;
	std::string	content = 
	"GET /cgi-bin/php.php/this-is-path-info?key1=valuee HTTP/1.1\r\n"
	"Host:localhost:8080\r\n"
	"User-Agent:    SuperBrowser/1.0\r\n"
	"Accept:\ttext/html\r\n"
	"Connection: keep-alive  \r\n"
	"X-Empty-Header:\r\n"
	"Accept-Language: de\r\n"
	"Connection: keep-alive  \r\n"
	"Accept-Language: en\r\n"
	"Accept-Language            : en\r\n"
	"\r\n"
	"BODYBODYBODYBODYBODYBODYBODYBODYBODYBODY";

	HttpRequest	request;
	CGI			cgi;

	request.parseHttpRequest(content);
	// try {
	// 	cgi.validateRequest(request);
	// 	cgi.initCGI(request);
	// 	cgi.pipeIO();
	// 	cgi.spawnProcess();
	// 	cgi.wait();
	// 	// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
	// }
	// catch (std::exception& e) {
	// 	std::cerr << e.what() << std::endl;
	// }
	return 0;
}
