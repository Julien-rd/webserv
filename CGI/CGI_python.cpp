#include "CGI.hpp"
#include <iostream>
#include <sstream>

bool CGI::setScriptAttributesPython(void) {
  for (size_t i = 0; i < this->_cgiConfigs.size(); i++) {
    if (this->_cgiConfigs.at(i).extension == ".py") {
      this->_executable = this->_cgiConfigs.at(i).executablePath;
      this->_argv.push_back(this->_cgiConfigs.at(i).executablePath);
      this->_argv.push_back("Pages/PasswordManager" + this->_request._uriData.path);//Fix: hardcoded, needs to be adaptable to serverConfig
      return true;
    }
  }
  return false; //FIX: pls add error message here
}

void CGI::setGETVariablesPython(void) {
  this->_envp[0] = this->_meta.request_method.c_str();
  this->_envp[1] = this->_meta.query_string.c_str();
  this->_envp[2] = this->_meta.script_name.c_str();
  this->_envp[3] = this->_meta.path_info.c_str();
  this->_envp[4] = this->_meta.server_name.c_str();
  this->_envp[5] = this->_meta.server_port.c_str();
  this->_envp[6] = this->_meta.server_protocol.c_str();
  this->_envp[7] = NULL;
}

void CGI::setPOSTVariablesPython(void) {
  this->_envp[0] = this->_meta.request_method.c_str();
  this->_envp[1] = this->_meta.content_length.c_str();
  this->_envp[2] = this->_meta.content_type.c_str();
  this->_envp[3] = this->_meta.script_name.c_str();
  this->_envp[4] = this->_meta.path_info.c_str();
  this->_envp[5] = this->_meta.server_name.c_str();
  this->_envp[6] = this->_meta.server_port.c_str();
  this->_envp[7] = this->_meta.server_protocol.c_str();
  this->_envp[8] = NULL;
}

void CGI::initMetaPython(void) {
  // this->_meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
  std::stringstream ss;
  ss << _request._contentLength;

  if (this->_request._method == "POST") {
    this->_meta.content_length = std::string("CONTENT_LENGTH=").append(ss.str());
  } else {
    this->_meta.content_length = std::string("CONTENT_LENGTH=").append("0");
  }
  this->_meta.content_type = std::string("CONTENT_TYPE=")
                                .append(this->_request._headers["content-type"]);
  // this->_meta.gateway_interface = "CGI/1.1";
  this->_meta.path_info =
      std::string("PATH_INFO=").append(this->_request._uriData.pathInfo);
  // this->_meta.path_translated = this->_meta.path_translated;
  this->_meta.query_string =
      std::string("QUERY_STRING=").append(parseQueryString(_request._uri));
  // this->_meta.remote_addr =
  //     std::string("REMOTE_ADDR=").append("1.1.1.1"); // TODO change this
  // this->_meta.remote_host = "";
  // this->_meta.remote_ident = "";
  // this->_meta.remote_user = "";
  this->_meta.request_method =
      std::string("REQUEST_METHOD=").append(_request._method);
  this->_meta.script_name = std::string("SCRIPT_NAME=").append(this->_scriptName);
  // this->_meta.server_name =
  // std::string("SERVER_NAME=").append(_request._headers.at("host")); // TODO
  // get this from result of config_parser instead std::cout <<
  // "_request._headers.at(host) is: " << _request._headers.at("host") <<
  // std::endl; std::cout <<
  // "_request._headers.at(host).substr(_request._headers).at(host) is: " <<
  // _request._headers.at("host") << std::endl; std::cout <<
  // "_request._headers.at(host).substr(_request._headers).at(host).find(:) + 1
  // is: " << _request._headers.at("host").find(':') + 1 << std::endl;
  this->_meta.server_port =
      std::string("SERVER_PORT=").append(this->_serverConfig.port);
  this->_meta.server_protocol =
      std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
  // this->_meta.server_software = "";
  // this->_meta.x = "";
}

bool CGI::initPythonScript(void) {
  this->initMetaPython();
  if (!this->setScriptAttributesPython())
      return false;
  if (_request._method == "GET") {
    this->setGETVariablesPython();
  } else if (_request._method == "POST") {
    this->setPOSTVariablesPython();
  } else {
    std::cerr << "Unknown method in CGI" << std::endl;
    return false; //ERROR
  }
  return true;
}
