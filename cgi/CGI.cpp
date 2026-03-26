#include "CGI.hpp"
#include "HttpRequest.hpp"
#include <exception>
#include <iostream>

#include <cstring>
#include <cstdio>

#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

CGI::CGI(void): envp(new const char*[10]),
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

pid_t	CGI::getPid(void) const {
	return this->pid;
}

void	CGI::setPid(pid_t pid) {
	this->pid = pid;
}

void	CGI::validateRequest(const HttpRequest& request) const {
	if (request._uri.compare(0, this->pythonScriptName.size(), this->pythonScriptName)
		&& request._uri.compare(0, this->phpScriptName.size(), this->phpScriptName)) { // TODO Or could replace this with a dynamic array of known scripts and check if URI matches one of them, then set a variable indicating that we will work with this specifi script for the rest of the execution oF CGI
		std::cout << "URI doesn't contain a known script" << std::endl;
		throw CGI::StandardException();
	}
	// TODO Validate minimum requirements needed for CGI execution (maybe headers for GET or POST. specific requirements for attributes of HttpRequest)???
}

void	CGI::setScriptAttributes(void) {
	if (this->scriptName == "/cgi-bin/python.py") {
		this->executable = (char *)"./python.py";
		this->argv[0] = (char *)"python.py";
	}
	else if (this->scriptName == "/cgi-bin/php.php") {
		this->executable = (char *)"./php.php";
		this->argv[0] = (char *)"php.php";
	}
	this->argv[1] = NULL;
}

void	CGI::setGETVariables(void) {
	this->envp[0] = this->meta.request_method.c_str();
	this->envp[1] = this->meta.query_string.c_str();
	this->envp[2] = this->meta.script_name.c_str();
	this->envp[3] = this->meta.path_info.c_str();
	this->envp[4] = this->meta.server_name.c_str();
	this->envp[5] = this->meta.server_port.c_str();
	this->envp[6] = this->meta.server_protocol.c_str();
	this->envp[7] = NULL;
}

void	CGI::setPOSTVariables(void) {

	this->envp[0] = this->meta.request_method.c_str();
	this->envp[1] = this->meta.content_length.c_str();
	this->envp[2] = this->meta.content_type.c_str();
	this->envp[3] = this->meta.script_name.c_str();
	this->envp[4] = this->meta.path_info.c_str();
	this->envp[5] = this->meta.server_name.c_str();
	this->envp[6] = this->meta.server_port.c_str();
	this->envp[7] = this->meta.server_protocol.c_str();
	this->envp[8] = NULL;
	
}

void	CGI::initMeta(const HttpRequest& request, const std::string& scriptName) {
	// this->meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
	std::stringstream	ss;
	ss << request._contentLength;

	this->meta.content_length = std::string("CONTENT_LENGTH=").append(ss.str()); // What about chunks?
	this->meta.content_type = std::string("CONTENT_TYPE=").append("text/html"); // ?? Default value is "US-ASCII"
	// this->meta.gateway_interface = "CGI/1.1";
	this->meta.path_info = std::string("PATH_INFO=").append(parsePathInfo(request._uri));
	// this->meta.path_translated = this->meta.path_translated;
	this->meta.query_string = std::string("QUERY_STRING=").append(parseQueryString(request._uri));
	// this->meta.remote_addr = "";
	// this->meta.remote_host = "";
	// this->meta.remote_ident = "";
	// this->meta.remote_user = "";
	this->meta.request_method = std::string("REQUEST_METHOD=").append(request._method);
	this->meta.script_name = std::string("SCRIPT_NAME=").append(scriptName);
	this->meta.server_name = std::string("SERVER_NAME=").append(request._headers.at("Host")); // TODO get this from result of config_parser instead
	this->meta.server_port = std::string("SERVER_PORT=").append(request._headers.at("Host").substr(request._headers.at("Host").find(':') + 1)); // TODO get this from result of config_parser instead
	this->meta.server_protocol = std::string("SERVER_PROTOCAL=").append("HTTP/1.1");
	// this->meta.server_software = "";
	// this->meta.x = "";
}

void	CGI::initCGI(const HttpRequest& request) {
	this->scriptName = getScriptName(request._uri, this->pythonScriptName, this->phpScriptName);

	this->initMeta(request, scriptName);
	this->setScriptAttributes();
	if (request._method == "GET") {
		this->setGETVariables();
	}
	else if (request._method == "POST") {
		this->setPOSTVariables();
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
	"\r\n";

	HttpRequest	request;
	CGI			cgi;

	request.parseHttpRequest(content);
	try {
		cgi.validateRequest(request);
		cgi.initCGI(request);
		cgi.pipeIO();
		cgi.spawnProcess();
		cgi.wait();
		// cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
