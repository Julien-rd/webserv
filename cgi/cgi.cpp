#include "cgi.hpp"
#include <exception>
#include <iostream>

#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

CGI::CGI(char *content):
	content(content) {}

CGI::~CGI(void) {}

pid_t	CGI::getPid(void) const {
	return this->pid;
}

void	CGI::setPid(pid_t pid) {
	this->pid = pid;
}

void	CGI::validateContent(void) const {
	// TODO VALIDATE HERE
	if (!this->content) {
		throw CGI::StandardException();
	}
}

// void	CGI::parseContent(char **newEnvp, char *content) {
// 	// TODO implement parsing
// 	// this->metaVariables.
// 	this->path = (char *)"./script.py";
// 	this->argv[0] = (char *)"script.py";
// 	this->argv[1] = NULL;
// 	this->envp = newEnvp;
// 	// this->envp[0] = (char *)"PATH=/usr/bin:/bin";
// 	// this->envp[1] = (char *)"CONTENT_LENGTH=TheMagnificent";
// 	// this->envp[2] = NULL;
// }

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
	if (this->pid > 0) {
		return ;
	}
	if (this->pid == -1) {
		throw CGI::StandardException();
	}
	if (this->pid == 0) {
		throw CGI::WaitException();
	}
}

// void	CGI::redirectIO(void) {

// 	if (dup2(STDOUT_FILENO, this->pipefd[1]) == -1) {
// 		throw CGI::StandardException();
// 	}
// 	if (dup2(STDIN_FILENO, this->pipefd[0]) == -1) {
// 		throw CGI::StandardException();
// 	}
// }

void	CGI::execute(void) {
	char	buf[10];

	// read(0, buf, 10);
	buf[9] = 0;
	printf("read (%s), now writing it back.\n", buf);
	fflush(stdout);
	// write(1, buf, 10);
	// write(1, "\n", 1);
	execve(this->path, this->argv, this->envp);
}

int	main(int argc, char **argv, char **envp) {
	(void)argc, void(argv), (void)envp;

	char	*content = (char *)"Content-Type: text/html\r\n\r\n<h1>Hello from CGI!</h1>\n";
	CGI		cgi(content);

	try {
		// cgi.parseRawHTTP(envp, content);
		cgi.validateContent();
		cgi.pipeIO();
		cgi.spawnProcess();
		cgi.execute();
		// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
	}
	catch (CGI::WaitException& e) {
		std::cout << e.what() << std::endl;
		if (waitpid(cgi.getPid(), NULL, 0) == -1) {
			std::cerr << "ERROR: waitpid(): " << strerror(errno) << std::endl;
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
