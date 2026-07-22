#include "CGI.hpp"

#include <iostream>
#include <sstream>

bool CGI::setScriptAttributesPython(void) {
    for (size_t i = 0; i < _cgiConfigs.size(); i++) {
        if (_cgiConfigs.at(i).extension == ".py") {
            _executable = _cgiConfigs.at(i).executablePath;
            _argv.push_back(_cgiConfigs.at(i).executablePath);
            _argv.push_back("Pages/PasswordManager" +
                            _request.getUriData()
                                .path);  // Fix: hardcoded, needs to be adaptable to serverConfig
            return true;
        }
    }
    return false;  // FIX: pls add error message here
}

void CGI::setGETVariablesPython(void) {
    _envp[0] = _meta.request_method.c_str();
    _envp[1] = _meta.query_string.c_str();
    _envp[2] = _meta.script_name.c_str();
    _envp[3] = _meta.path_info.c_str();
    _envp[4] = _meta.server_name.c_str();
    _envp[5] = _meta.server_port.c_str();
    _envp[6] = _meta.server_protocol.c_str();
    _envp[7] = NULL;
}

void CGI::setPOSTVariablesPython(void) {
    _envp[0] = _meta.request_method.c_str();
    _envp[1] = _meta.content_length.c_str();
    _envp[2] = _meta.content_type.c_str();
    _envp[3] = _meta.script_name.c_str();
    _envp[4] = _meta.path_info.c_str();
    _envp[5] = _meta.server_name.c_str();
    _envp[6] = _meta.server_port.c_str();
    _envp[7] = _meta.server_protocol.c_str();
    _envp[8] = NULL;
}

void CGI::initMetaPython(void) {
    // _meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
    std::stringstream ss;
    ss << _request.getContentLength();

    if (_request.getMethod() == "POST") {
        _meta.content_length = std::string("CONTENT_LENGTH=").append(ss.str());
    } else {
        _meta.content_length = std::string("CONTENT_LENGTH=").append("0");
    }
    _meta.content_type =
        std::string("CONTENT_TYPE=").append(_request.getHeaders().at("content-type"));
    // _meta.gateway_interface = "CGI/1.1";
    _meta.path_info = std::string("PATH_INFO=").append(_request.getUriData().pathInfo);
    // _meta.path_translated = _meta.path_translated;
    _meta.query_string = std::string("QUERY_STRING=").append(parseQueryString(_request.getUri()));
    // _meta.remote_addr =
    //     std::string("REMOTE_ADDR=").append("1.1.1.1"); // TODO change this
    // _meta.remote_host = "";
    // _meta.remote_ident = "";
    // _meta.remote_user = "";
    _meta.request_method = std::string("REQUEST_METHOD=").append(_request.getMethod());
    _meta.script_name = std::string("SCRIPT_NAME=").append(_scriptName);
    // _meta.server_name =
    // std::string("SERVER_NAME=").append(_request._headers.at("host")); // TODO
    // get this from result of config_parser instead std::cout <<
    // "_request._headers.at(host) is: " << _request._headers.at("host") <<
    // std::endl; std::cout <<
    // "_request._headers.at(host).substr(_request._headers).at(host) is: " <<
    // _request._headers.at("host") << std::endl; std::cout <<
    // "_request._headers.at(host).substr(_request._headers).at(host).find(:) + 1
    // is: " << _request._headers.at("host").find(':') + 1 << std::endl;
    _meta.server_port = std::string("SERVER_PORT=").append(_serverConfig.port);
    _meta.server_protocol = std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
    // _meta.server_software = "";
    // _meta.x = "";
}

bool CGI::initPythonScript(void) {
    initMetaPython();
    if (!setScriptAttributesPython())
        return false;
    if (_request.getMethod() == "GET") {
        setGETVariablesPython();
    } else if (_request.getMethod() == "POST") {
        setPOSTVariablesPython();
    } else {
        std::cerr << "Unknown method in CGI" << std::endl;
        return false;  // ERROR
    }
    return true;
}
