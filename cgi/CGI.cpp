#include "CGI.hpp"
#include "HttpRequest.hpp"
#include <exception>
#include <iostream>
#include <iterator>

#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SCRIPT_NAME "/cgi-bin/script.py"

CGI::CGI(void) {}

CGI::~CGI(void) {
	delete []this->envp;
}

pid_t	CGI::getPid(void) const {
	return this->pid;
}

void	CGI::setPid(pid_t pid) {
	this->pid = pid;
}

void	CGI::validateRequest(const HttpRequest& request) const {
	std::string	scriptName = SCRIPT_NAME;
	if (request._uri.compare(0, scriptName.size(), scriptName)) { // TODO Or could replace this with a dynamic array of known scripts and check if URI matches one of them, then set a variable indicating that we will work with this specifi script for the rest of the execution oF CGI
		std::cout << "URI is not a known script (\"/cgi-bin/script.py\")";
		throw CGI::StandardException();
	}
	// TODO Do we need more validations???
}

std::string	parsePathInfo(const std::string& _uri) {
	// const std::string	scriptName = SCRIPT_NAME;
	const size_t	pathInfoPos = _uri.find(SCRIPT_NAME) + std::string(SCRIPT_NAME).size();
	size_t			queryStringPos = _uri.find('?');

	// This is commented out because it should already be validated and confirmed to be one of the scripts (if we implement that) in valduateRequest()
	if (pathInfoPos == std::string::npos) {
		std::cerr << "ERROR: maybe unknown script path in parsePathInfo(). Avoid reaching here in execution" << std::endl;
		throw CGI::StandardException();
	}
	if (queryStringPos != std::string::npos) {
		if (queryStringPos > pathInfoPos) {
			std::cout << "DEBUG: found '?' before PATH_INFO in parsePathInfo(). Avoid reaching here in execution" << std::endl;
			throw CGI::StandardException();
		}
	}
	else {
		queryStringPos = std::string(SCRIPT_NAME).size();
	}
	std::string res(_uri.substr(pathInfoPos, queryStringPos - pathInfoPos));
	std::cout << "parsed PATH_INFO as: " << res << std::endl;
	return res;
}

std::string	parseQueryString(const std::string& _uri) {
	const std::string	queryString;
	const size_t		queryStringPos = _uri.find('?');

	if (queryStringPos == std::string::npos) {
		return "";
	}
	return _uri.substr(queryStringPos + 1);
}

void	CGI::setGETVariables(const HttpRequest& request) {
	this->path = (char *)"./script.py";
	this->argv[0] = (char *)"script.py";
	this->argv[1] = NULL;
	this->envp[0] = std::string("REQUEST_METHOD=").append(request._method).c_str();
	this->envp[1] = std::string("QUERY_STRING=").append(parseQueryString(request._uri)).c_str();
	this->envp[2] = std::string("SCRIPT_NAME=").append(SCRIPT_NAME).c_str();

	this->envp[3] = std::string("PATH_INFO=").append(parsePathInfo(request._uri)).c_str();
	// this->envp[2] = std::string("SERVER_PROTOCOL=").c_str();
	// this->envp[2] = std::string("SERVER_NAME=").c_str();
	// this->envp[2] = std::string("SERVER_PORT=").c_str();
	this->envp[4] = NULL;
}

void	CGI::setPOSTVariables(const HttpRequest& request) {
	this->path = (char *)"./script.py";
	this->argv[0] = (char *)"script.py";
	this->argv[1] = NULL;
	this->envp[0] = std::string("REQUEST_METHOD=").append(request._method).c_str();
	// this->envp[1] = std::string("CONTENT_LENGTH=")
	// this->envp[2] = std::string("CONTENT_TYPE=")
	this->envp[1] = NULL;
	
}

void	CGI::setMetaVariables(const HttpRequest& request) {
	(void)request;
	// this->metaVariables.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
	// this->metaVariables.content_length = request._contentLength; // What about chunks?
	// this->metaVariables.content_type = ""; // ?? Default value is "US-ASCII"
	// this->metaVariables.gateway_interface = "CGI/1.1";
	// this->metaVariables.path_info = parsePathInfo(request._uri);
	// this->metaVariables.path_translated = this->metaVariables.path_translated;
	// this->metaVariables.query_string = parseQueryString(request._uri);
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
}

void	CGI::initCGI(const char **parentEnvp, const HttpRequest& request) {
	// TODO implement parsing

	(void)parentEnvp;
	this->envp = new const char *[10];
	this->setMetaVariables(request); // TODO Do we need this??
	if (request._method == "GET") {
		this->setGETVariables(request);
	}
	else if (request._method == "POST") {
		this->setPOSTVariables(request);
	}
	else {
		std::cerr << "Unknown method in CGI" << std::endl;
		throw CGI::StandardException();	
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
	"GET /cgi-bin/script.py/this-is-path-info HTTP/1.1\r\n"
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
