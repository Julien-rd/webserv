#include "CGI.hpp"
#include <iostream>
#include <sstream>

bool CGI::setScriptAttributesPhp(void) {
  for (size_t i = 0; i < this->cgiConfigs.size(); i++) {
    if (this->cgiConfigs.at(i).extension == ".php") {
      this->executable = this->cgiConfigs.at(i).executablePath;
      this->argv.push_back(this->cgiConfigs.at(i).executablePath);
      this->argv.push_back("Path/PasswordManager" + this->request._uriData.path); //Fix: hardcoded, needs to be adaptable to serverConfig
      return true;
    }
  }
  return false; //ERROR here
}

void CGI::setGETVariablesPhp(void) {
  this->envp[0] = this->meta.request_method.c_str();
  this->envp[1] = this->meta.query_string.c_str();
  this->envp[2] = this->meta.script_name.c_str();
  this->envp[3] = this->meta.path_info.c_str();
  this->envp[4] = this->meta.server_name.c_str();
  this->envp[5] = this->meta.server_port.c_str();
  this->envp[6] = this->meta.server_protocol.c_str();
  /* php specific */
  this->envp[7] = "REDIRECT_STATUS=1";
  this->envp[8] = this->meta.script_filename.c_str();
  this->envp[9] = this->meta.remote_addr.c_str();
  this->envp[10] = NULL;
}

void CGI::setPOSTVariablesPhp(void) {

  this->envp[0] = this->meta.request_method.c_str();
  this->envp[1] = this->meta.content_length.c_str();
  this->envp[2] = this->meta.content_type.c_str();
  this->envp[3] = this->meta.script_name.c_str();
  this->envp[4] = this->meta.path_info.c_str();
  this->envp[5] = this->meta.server_name.c_str();
  this->envp[6] = this->meta.server_port.c_str();
  this->envp[7] = this->meta.server_protocol.c_str();
  /* php specific */
  this->envp[8] = "REDIRECT_STATUS=1";
  this->envp[9] = this->meta.script_filename.c_str();
  this->envp[10] = this->meta.remote_addr.c_str();
  this->envp[11] = NULL;
}

void CGI::initMetaPhp(void) {
  // this->meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
  std::stringstream ss;
  ss << request._contentLength;

  if (this->request._method == "POST") {
    this->meta.content_length =
        std::string("CONTENT_LENGTH=").append(ss.str()); // What about chunks?
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
  //     std::string("REMOTE_ADDR=").append(this->serverConfig.port);
  // this->meta.remote_host = "";
  // this->meta.remote_ident = "";
  // this->meta.remote_user = "";
  this->meta.request_method =
      std::string("REQUEST_METHOD=").append(request._method);
  this->meta.script_name = std::string("SCRIPT_NAME=").append(this->scriptName);
  // this->meta.server_name =
  //     std::string("SERVER_NAME=")
  //         .append(request._headers.at(
  //             "Host")); // TODO get this from result of config_parser instead
  this->meta.server_port =
      std::string("SERVER_PORT=").append(this->serverConfig.port);
  this->meta.server_protocol =
      std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
  // this->meta.script_filename =
  // std::string("SCRIPT_FILENAME=").append("/home/jel-ghna/github/webserv/cgi-bin/php.php");
  // // TODO maybe make this better
  this->meta.script_filename =
      std::string("SCRIPT_FILENAME=cgi-bin")
          .append(this->request._uriData.path); // TODO maybe make this better
                                                // this->meta.server_software =
                                                // ""; this->meta.x = "";
}

bool CGI::initPhpScript(void) {
  this->initMetaPhp();
  if (!this->setScriptAttributesPhp())
      return false;
  if (request._method == "GET") {
    this->setGETVariablesPhp();
  } else if (request._method == "POST") {
    this->setPOSTVariablesPhp();
  } else {
    std::cerr << "Unknown method in CGI" << std::endl;
    return false; 
  }
  return true;
}
