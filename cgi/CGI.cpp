#include "CGI.hpp"
#include "../HttpRequest.hpp"
#include <exception>
#include <iostream>

#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

CGI::CGI(void) {}

CGI::~CGI(void) {}

pid_t	CGI::getPid(void) const {
	return this->pid;
}

void	CGI::setPid(pid_t pid) {
	this->pid = pid;
}

void	CGI::validateRequest(const HttpRequest& request) const {
	// TODO VALIDATE HERE

	if (request._uri != "/cgi-bin/script.py") {
		std::cout << "URI is not a script (\"/cgi-bin/script.py\")";
		throw CGI::StandardException();
	}
}

std::string	parsePathInfo(const std::string& _uri) {
	// TODO Maybe also handle errors here, instead of in validateRequest() ?

	const std::string	scriptPath = "/cgi-bin/script.py";
	const size_t		scriptPathPos = _uri.find(scriptPath);

	if (scriptPathPos == std::string::npos) {
		std::cerr << "ERROR: maybe unknown script path" << std::endl;
		throw CGI::StandardException();
	}
	return _uri.substr(scriptPathPos + scriptPath.size());
}

std::string	parseQueryString(const std::string& _uri) {
	const std::string	queryString;
	const size_t		queryStringPos = _uri.find('?');

	if (queryStringPos == std::string::npos) {
		return "";
	}
	return _uri.substr(queryStringPos + 1);
}

void	CGI::initCGI(const char **newEnvp, const HttpRequest& request) {
	// TODO implement parsing

	(void)newEnvp;
	this->envp = (const char **)malloc(sizeof(char *) * 18);
	if (!this->envp) {
		throw CGI::StandardException();
	}
	// this->metaVariables.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
	// this->metaVariables.content_length = request._contentLength; // What about chunks?
	// this->metaVariables.content_type = ""; // ?? Default value is "US-ASCII"
	// this->metaVariables.gateway_interface = "CGI/1.1";
	this->metaVariables.path_info = parsePathInfo(request._uri);
	// this->metaVariables.path_translated = this->metaVariables.path_translated;
	this->metaVariables.query_string = parseQueryString(request._uri);
	// this->metaVariables.remote_addr = "";
	// this->metaVariables.remote_host = "";
	// this->metaVariables.remote_ident = "";
	// this->metaVariables.remote_user = "";
	// this->metaVariables.request_method = request._method;
	// this->metaVariables.script_name = "script.py";
	// this->metaVariables.server_name = "";
	// this->metaVariables.server_port = "";
	// this->metaVariables.server_protocol = "";
	// this->metaVariables.server_software = "";
	// this->metaVariables.x = "";
	this->path = (char *)"./script.py";
	this->argv[0] = (char *)"script.py";
	this->argv[1] = NULL;
	// this->envp = newEnvp;
	this->envp[0] = (std::string(request._method).insert(0, "REQUEST_METHOD=")).c_str();
	this->envp[1] = NULL;
	// this->envp[0] = (char *)"PATH=/usr/bin:/bin"; // TODO make this dynamic maybe ?
	// this->envp[1] = (char *)"CONTENT_LENGTH=TheMagnificent";
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
	// char	buf[10];

	// read(0, buf, 10);
	// buf[9] = 0;
	// printf("read (%s), now writing it back.\n", buf);
	// fflush(stdout);
	// write(1, buf, 10);
	// write(1, "\n", 1);
	execve("./script.py", this->argv, const_cast<char **>(this->envp));
}

int	main(const int argc, const char **argv, const char **envp) {
	(void)argc, void(argv), (void)envp;
	std::string	content = 
	"GET /cgi-bin/script.py HTTP/1.1\r\n"
	"Host:localhost:8080\r\n"
	"User-Agent:    SuperBrowser/1.0\r\n"
	"Accept:\ttext/html\r\n"
	"Connection: keep-alive  \r\n"
	"X-Empty-Header:\r\n"
	"Accept-Language: de\r\n"
	"Connection: keep-alive  \r\n"
	"Accept-Language: en\r\n"
	"Accept-Language            : en\r\n"
	"\r\n";

	HttpRequest	request;
	CGI			cgi;

	request.parseHttpRequest(content);
	try {
		cgi.validateRequest(request);
		cgi.initCGI(envp, request);
		cgi.pipeIO();
		cgi.spawnProcess();
		cgi.execute();
		// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
	}
	catch (CGI::WaitException& e) {
		std::cout << e.what() << std::endl;
		if (waitpid(cgi.getPid(), NULL, 0) == -1) {
			std::cout << "ERROR: waitpid(): " << strerror(errno) << std::endl; // TODO Handle real errors and success waits
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
