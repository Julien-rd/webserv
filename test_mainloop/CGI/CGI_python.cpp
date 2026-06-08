#include "CGI.hpp"
#include <iostream>
#include <sstream>

void CGI::setScriptAttributesPython(void) {
  for (size_t i = 0; i < this->cgiConfigs.size(); i++) {
    if (this->cgiConfigs.at(i).extension == ".py") {
      this->executable = this->cgiConfigs.at(i).executablePath;
      this->argv.push_back(this->cgiConfigs.at(i).executablePath);
      break;
    } else if (i + 1 == this->cgiConfigs.size()) {
      throw CGI::StandardException();
    }
  }
  this->argv.push_back("cgi-bin" + this->request._uriData.path);
}

void CGI::setGETVariablesPython(void) {
  this->envp[0] = this->meta.request_method.c_str();
  this->envp[1] = this->meta.query_string.c_str();
  this->envp[2] = this->meta.script_name.c_str();
  this->envp[3] = this->meta.path_info.c_str();
  this->envp[4] = this->meta.server_name.c_str();
  this->envp[5] = this->meta.server_port.c_str();
  this->envp[6] = this->meta.server_protocol.c_str();
  this->envp[7] = NULL;
}

void CGI::setPOSTVariablesPython(void) {
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

void CGI::initMetaPython(void) {
  // this->meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
  std::stringstream ss;
  ss << request._contentLength;

  if (this->request._method == "POST") {
    this->meta.content_length = std::string("CONTENT_LENGTH=").append(ss.str());
  } else {
    this->meta.content_length = std::string("CONTENT_LENGTH=").append("0");
  }
  this->meta.content_type = std::string("CONTENT_TYPE=")
                                .append(this->request._headers["content-type"]);
  // this->meta.gateway_interface = "CGI/1.1";
  this->meta.path_info =
      std::string("PATH_INFO=").append(this->request._uriData.pathInfo);
  // this->meta.path_translated = this->meta.path_translated;
  this->meta.query_string =
      std::string("QUERY_STRING=").append(parseQueryString(request._uri));
  // this->meta.remote_addr =
  //     std::string("REMOTE_ADDR=").append("1.1.1.1"); // TODO change this
  // this->meta.remote_host = "";
  // this->meta.remote_ident = "";
  // this->meta.remote_user = "";
  this->meta.request_method =
      std::string("REQUEST_METHOD=").append(request._method);
  this->meta.script_name = std::string("SCRIPT_NAME=").append(this->scriptName);
  // this->meta.server_name =
  // std::string("SERVER_NAME=").append(request._headers.at("host")); // TODO
  // get this from result of config_parser instead std::cout <<
  // "request._headers.at(host) is: " << request._headers.at("host") <<
  // std::endl; std::cout <<
  // "request._headers.at(host).substr(request._headers).at(host) is: " <<
  // request._headers.at("host") << std::endl; std::cout <<
  // "request._headers.at(host).substr(request._headers).at(host).find(:) + 1
  // is: " << request._headers.at("host").find(':') + 1 << std::endl;
  this->meta.server_port =
      std::string("SERVER_PORT=").append(this->serverConfig.port);
  this->meta.server_protocol =
      std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
  // this->meta.server_software = "";
  // this->meta.x = "";
}

void CGI::initPythonScript(void) {
  this->initMetaPython();
  this->setScriptAttributesPython();
  if (request._method == "GET") {
    this->setGETVariablesPython();
  } else if (request._method == "POST") {
    this->setPOSTVariablesPython();
  } else {
    std::cerr << "Unknown method in CGI" << std::endl;
    throw CGI::StandardException();
  }
}
