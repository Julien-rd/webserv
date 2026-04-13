#include "CGI.hpp"
#include <iostream>
#include <sstream>

void	CGI::setScriptAttributesPython(void) {
	this->executable = (char *)"/usr/bin/python3";
	this->argv[0] = (char *)"/usr/bin/python3"; // TODO This is hardcoded!!
	this->argv[1] = (char *)"../cgi-bin/python.py"; // TODO This is hardcoded!!
	this->argv[2] = NULL;
}

void	CGI::setGETVariablesPython(void) {
	this->envp[0] = this->meta.request_method.c_str();
	this->envp[1] = this->meta.query_string.c_str();
	this->envp[2] = this->meta.script_name.c_str();
	this->envp[3] = this->meta.path_info.c_str();
	this->envp[4] = this->meta.server_name.c_str();
	this->envp[5] = this->meta.server_port.c_str();
	this->envp[6] = this->meta.server_protocol.c_str();
	this->envp[7] = NULL;
}

void	CGI::setPOSTVariablesPython(void) {

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

void	CGI::initMetaPython(void) {
	// this->meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
	std::stringstream	ss;
	ss << request._contentLength;

	this->meta.content_length = std::string("CONTENT_LENGTH=").append(ss.str());
	this->meta.content_type = std::string("CONTENT_TYPE=").append("text/html"); // ?? Default value is "US-ASCII"
	// this->meta.gateway_interface = "CGI/1.1";
	this->meta.path_info = std::string("PATH_INFO=").append(parsePathInfo(request._uri, this->scriptName));
	// this->meta.path_translated = this->meta.path_translated;
	this->meta.query_string = std::string("QUERY_STRING=").append(parseQueryString(request._uri));
	this->meta.remote_addr = std::string("REMOTE_ADDR=").append("1.1.1.1"); // TODO change this
	// this->meta.remote_host = "";
	// this->meta.remote_ident = "";
	// this->meta.remote_user = "";
	this->meta.request_method = std::string("REQUEST_METHOD=").append(request._method);
	this->meta.script_name = std::string("SCRIPT_NAME=").append(this->scriptName);
	this->meta.server_name = std::string("SERVER_NAME=").append(request._headers.at("Host")); // TODO get this from result of config_parser instead
	this->meta.server_port = std::string("SERVER_PORT=").append(request._headers.at("Host").substr(request._headers.at("Host").find(':') + 1)); // TODO get this from result of config_parser instead
	this->meta.server_protocol = std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
	// this->meta.server_software = "";
	// this->meta.x = "";
}

void	CGI::initPythonScript(void) {
	this->initMetaPython();
	this->setScriptAttributesPython();
	if (request._method == "GET") {
		this->setGETVariablesPython();
	}
	else if (request._method == "POST") {
		this->setPOSTVariablesPython();
	}
	else {
		std::cerr << "Unknown method in CGI" << std::endl;
		throw CGI::StandardException();
	}
}
