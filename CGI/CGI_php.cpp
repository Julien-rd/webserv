#include "../Utils/Macros.hpp"
#include "CGI.hpp"

#include <iostream>
#include <sstream>

bool CGI::setScriptAttributesPhp(void) {
    for (size_t i = 0; i < _cgiConfigs->size(); i++) {
        if (_cgiConfigs->at(i).extension == ".php") {
            _executable = _cgiConfigs->at(i).executablePath;
            _argv.push_back(_cgiConfigs->at(i).executablePath);
            _argv.push_back(ROOT_FOLDER + _serverConfig->locations.at(0).root +
                            _request->getUriData().path);
            return true;
        }
    }
    return false;  // ERROR here
}

void CGI::setGETVariablesPhp(void) {
    _envp[0] = _meta.request_method.c_str();
    _envp[1] = _meta.query_string.c_str();
    _envp[2] = _meta.script_name.c_str();
    _envp[3] = _meta.path_info.c_str();
    _envp[4] = _meta.server_name.c_str();
    _envp[5] = _meta.server_port.c_str();
    _envp[6] = _meta.server_protocol.c_str();
    /* php specific */
    _envp[7] = "REDIRECT_STATUS=1";
    _envp[8] = _meta.script_filename.c_str();
    _envp[9] = _meta.remote_addr.c_str();
    _envp[10] = NULL;
}

void CGI::setPOSTVariablesPhp(void) {

    _envp[0] = _meta.request_method.c_str();
    _envp[1] = _meta.content_length.c_str();
    _envp[2] = _meta.content_type.c_str();
    _envp[3] = _meta.script_name.c_str();
    _envp[4] = _meta.path_info.c_str();
    _envp[5] = _meta.server_name.c_str();
    _envp[6] = _meta.server_port.c_str();
    _envp[7] = _meta.server_protocol.c_str();
    /* php specific */
    _envp[8] = "REDIRECT_STATUS=1";
    _envp[9] = _meta.script_filename.c_str();
    _envp[10] = _meta.remote_addr.c_str();
    _envp[11] = NULL;
}

void CGI::initMetaPhp(void) {
    try {
        // _meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
        std::stringstream ss;
        ss << _request->getContentLength();

        if (_request->getMethod() == "POST") {
            _meta.content_length =
                std::string("CONTENT_LENGTH=").append(ss.str());  // What about chunks?
        } else {
            _meta.content_length = std::string("CONTENT_LENGTH=").append("0");
        }
        _meta.content_type =
            std::string("CONTENT_TYPE=").append(_request->getHeaders().at("content-type"));
        // _meta.gateway_interface = "CGI/1.1";
        _meta.path_info = std::string("PATH_INFO=").append(_request->getUriData().pathInfo);
        // _meta.path_translated = _meta.path_translated;
        _meta.query_string =
            std::string("QUERY_STRING=").append(parseQueryString(_request->getUri()));
        // _meta.remote_addr =
        //     std::string("REMOTE_ADDR=").append(serverConfig.port);
        // _meta.remote_host = "";
        // _meta.remote_ident = "";
        // _meta.remote_user = "";
        _meta.request_method = std::string("REQUEST_METHOD=").append(_request->getMethod());
        _meta.script_name = std::string("SCRIPT_NAME=").append(_scriptName);
        // _meta.server_name =
        //     std::string("SERVER_NAME=")
        //         .append(_request->_headers.at(
        //             "Host")); // TODO get this from result of config_parser instead
        _meta.server_port = std::string("SERVER_PORT=").append(_serverConfig->port);
        _meta.server_protocol = std::string("SERVER_PROTOCOL=").append("HTTP/1.1");
        // _meta.script_filename =
        // std::string("SCRIPT_FILENAME=").append("/home/jel-ghna/github/webserv/cgi-bin/php.php");
        // // TODO maybe make this better
        _meta.script_filename =
            std::string("SCRIPT_FILENAME=cgi-bin")
                .append(_request->getUriData().path);  // TODO maybe make this better
                                                       // _meta.server_software =
                                                       // ""; _meta.x = "";
    } catch (std::exception &e) {
        std::cerr << "unexpected erorr in initMetaPhp(): " << e.what() << std::endl;
    }
}

bool CGI::initPhpScript(void) {
    initMetaPhp();
    if (!setScriptAttributesPhp())
        return false;
    if (_request->getMethod() == "GET") {
        setGETVariablesPhp();
    } else if (_request->getMethod() == "POST") {
        setPOSTVariablesPhp();
    } else {
        std::cerr << "Unknown method in CGI" << std::endl;
        return false;
    }
    return true;
}
