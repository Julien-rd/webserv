#include "CGI.hpp"
#include <iostream>
#include <sstream>

bool CGI::setScriptAttributesPhp(void) {
  for (size_t i = 0; i < this->_cgiConfigs.size(); i++) {
    if (this->_cgiConfigs.at(i).extension == ".php") {
      this->_executable = this->_cgiConfigs.at(i).executablePath;
      this->_argv.push_back(this->_cgiConfigs.at(i).executablePath);
      this->_argv.push_back("Path/PasswordManager" + this->_request._uriData.path); //Fix: hardcoded, needs to be adaptable to serverConfig
      return true;
    }
  }
  return false; //ERROR here
}

void CGI::setGETVariablesPhp(void) {
  this->_envp[0] = this->_meta.request_method.c_str();
  this->_envp[1] = this->_meta.query_string.c_str();
  this->_envp[2] = this->_meta.script_name.c_str();
  this->_envp[3] = this->_meta.path_info.c_str();
  this->_envp[4] = this->_meta.server_name.c_str();
  this->_envp[5] = this->_meta.server_port.c_str();
  this->_envp[6] = this->_meta.server_protocol.c_str();
  /* php specific */
  this->_envp[7] = "REDIRECT_STATUS=1";
  this->_envp[8] = this->_meta.script_filename.c_str();
  this->_envp[9] = this->_meta.remote_addr.c_str();
  this->_envp[10] = NULL;
}

void CGI::setPOSTVariablesPhp(void) {

  this->_envp[0] = this->_meta.request_method.c_str();
  this->_envp[1] = this->_meta.content_length.c_str();
  this->_envp[2] = this->_meta.content_type.c_str();
  this->_envp[3] = this->_meta.script_name.c_str();
  this->_envp[4] = this->_meta.path_info.c_str();
  this->_envp[5] = this->_meta.server_name.c_str();
  this->_envp[6] = this->_meta.server_port.c_str();
  this->_envp[7] = this->_meta.server_protocol.c_str();
  /* php specific */
  this->_envp[8] = "REDIRECT_STATUS=1";
  this->_envp[9] = this->_meta.script_filename.c_str();
  this->_envp[10] = this->_meta.remote_addr.c_str();
  this->_envp[11] = NULL;
}

void CGI::initMetaPhp(void) {
  // this->_meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
  std::stringstream ss;
  ss << _request._contentLength;

  if (this->_request._method == "POST") {
    this->_meta.content_length =
        std::string("CONTENT_LENGTH=").append(ss.str()); // What about chunks?
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
  //     std::string("REMOTE_ADDR=").append(this->serverConfig.port);
  // this->_meta.remote_host = "";
  // this->_meta.remote_ident = "";
  // this->_meta.remote_user = "";
  this->_meta.request_method =
      std::string("REQUEST_METHOD=").append(_request._method);
  this->_meta.script_name = std::string("SCRIPT_NAME=").append(this->_scriptName);
  // this->_meta.server_name =
  //     std::string("SERVER_NAME=")
  //         .append(_request._headers.at(
  //             "Host")); // TODO get this from result of config_parser instead
  this->_meta.server_port =
      std::string("SERVER_PORT=").append(this->_serverConfig.port);
  this->_meta.server_protocol =
      std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
  // this->_meta.script_filename =
  // std::string("SCRIPT_FILENAME=").append("/home/jel-ghna/github/webserv/cgi-bin/php.php");
  // // TODO maybe make this better
  this->_meta.script_filename =
      std::string("SCRIPT_FILENAME=cgi-bin")
          .append(this->_request._uriData.path); // TODO maybe make this better
                                                // this->_meta.server_software =
                                                // ""; this->_meta.x = "";
}

bool CGI::initPhpScript(void) {
  this->initMetaPhp();
  if (!this->setScriptAttributesPhp())
      return false;
  if (_request._method == "GET") {
    this->setGETVariablesPhp();
  } else if (_request._method == "POST") {
    this->setPOSTVariablesPhp();
  } else {
    std::cerr << "Unknown method in CGI" << std::endl;
    return false; 
  }
  return true;
}
